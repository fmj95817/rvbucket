#!/usr/bin/env python3
"""
RVBucket Web UI v2 — aiohttp server with WebSocket + xterm.js frontend.

Replaces the v1 Flask+SSE+HTTP-POST design. The protocol with the C-model
sim (via stdin/stdout socketpair) is unchanged so ui_web.c needs only a
one-line path edit.

Protocol (stdin/stdout):
  uart:XX\n   — sim → web: UART byte output (hex)
  gpio:XXXX\n — sim → web: 16-bit GPIO value (hex)
  in:XX\n     — web → sim: UART byte input  (hex)

WebSocket JSON (server ↔ browser):
  → {"type":"uart",  "data":"416263"}         // batched UART bytes in hex
  → {"type":"gpio",  "data":"00ff"}           // GPIO value in hex
  ← {"type":"uart_in","data":"48656c6c6f"}    // UART input bytes in hex

Key features over v1:
  - 64 KB ring-buffer replays recent UART output to late-joining /
    reconnecting clients so no boot-log output is lost.
  - Line-buffered stdin for low latency.
  - UART byte batching to cut WebSocket message overhead.
  - Parallel broadcast (one slow client doesn't stall others).
  - SO_REUSEADDR + SO_REUSEPORT for fast server restart on same port.

Requires: aiohttp  (pip install aiohttp)
"""
import asyncio
import json
import os
import socket
import sys
import threading

from aiohttp import web

# ── port discovery ────────────────────────────────────────────────────────

def find_free_port(start: int = 5000) -> int:
    for port in range(start, start + 100):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            if s.connect_ex(("127.0.0.1", port)) != 0:
                return port
    return start

# ── global state ───────────────────────────────────────────────────────────

clients: set["web.WebSocketResponse"] = set()
stdin_queue: "asyncio.Queue[str]" = asyncio.Queue(maxsize=4096)
_loop: "asyncio.AbstractEventLoop | None" = None

# 64 KB ring-buffer for recent UART output — replayed to clients that
# connect after the sim has already started producing output.
_UART_HISTORY_SIZE = 65536
_uart_history = bytearray(_UART_HISTORY_SIZE)
_uart_history_wr = 0       # next write position
_uart_history_full = False

# last known GPIO value (sent to new clients on connect)
_last_gpio: str | None = None

# ── history helpers ───────────────────────────────────────────────────────

def _record_uart_hex(hex_str: str) -> None:
    """Append hex-encoded UART bytes to the ring-buffer."""
    global _uart_history_wr, _uart_history_full
    raw = bytes.fromhex(hex_str)
    for b in raw:
        _uart_history[_uart_history_wr] = b
        _uart_history_wr += 1
        if _uart_history_wr >= _UART_HISTORY_SIZE:
            _uart_history_wr = 0
            _uart_history_full = True


def _get_uart_history_hex() -> str:
    """Return the ring-buffer contents as a hex string, with any trailing
    incomplete ANSI escape sequence removed.

    Without this, an escape sequence split across the history/live-data
    boundary would leave xterm.js in a partial-parse state.  The user's
    first keystroke echo would then combine with the incomplete sequence
    and render as a spurious '?' character.
    """
    if not _uart_history_full:
        raw = bytes(_uart_history[:_uart_history_wr])
    else:
        tail = bytes(_uart_history[_uart_history_wr:])
        head = bytes(_uart_history[:_uart_history_wr])
        raw = tail + head
    return _clean_history_tail(raw).hex()


def _clean_history_tail(data: bytes, max_lookback: int = 512) -> bytes:
    """If *data* ends with an incomplete ANSI escape sequence, truncate it.

    Only inspects the last *max_lookback* bytes for an un-terminated
    ``ESC`` / ``ESC [`` (CSI) / ``ESC ]`` (OSC) sequence.  Normal
    characters and completely-terminated sequences pass through unchanged.
    """
    if not data:
        return data

    # only search the tail — escape sequences are short
    lookback = min(len(data), max_lookback)
    tail = data[-lookback:]

    # find the rightmost ESC
    esc_pos = tail.rfind(b"\x1b")
    if esc_pos < 0:
        return data  # no ESC anywhere near the end — safe

    seq = tail[esc_pos:]          # bytes from that ESC to end of history
    if _escape_seq_is_complete(seq):
        return data                # complete — safe

    # incomplete — discard everything from that ESC onward
    cutoff = len(data) - lookback + esc_pos
    return data[:cutoff]


def _escape_seq_is_complete(seq: bytes) -> bool:
    """Return True if *seq* (starting with ``\\x1b``) forms a complete,
    well-formed ANSI escape sequence."""
    if len(seq) < 2:
        return False               # lone ESC

    introducer = seq[1]

    if introducer == 0x5B:         # ESC [  → CSI
        # CSI ends with a final byte in 0x40–0x7E
        for b in seq[2:]:
            if 0x40 <= b <= 0x7E:
                return True
            # parameter (0x30–0x3F, 0x3B) / intermediate (0x20–0x2F) bytes
            if not (0x20 <= b <= 0x3F or b == 0x3B):
                return False       # garbage byte inside CSI
        return False               # no final byte

    if introducer == 0x5D:         # ESC ]  → OSC
        # OSC ends with BEL (0x07) or ST (ESC \)
        for i in range(2, len(seq)):
            if seq[i] == 0x07:
                return True
            if seq[i] == 0x1B and i + 1 < len(seq) and seq[i + 1] == 0x5C:
                return True
        return False

    # ESC P / ESC _ / ESC ^  (DCS, APC, PM) — same terminator as OSC
    if introducer in (0x50, 0x5F, 0x5E):
        for i in range(2, len(seq)):
            if seq[i] == 0x07:
                return True
            if seq[i] == 0x1B and i + 1 < len(seq) and seq[i + 1] == 0x5C:
                return True
        return False

    # simple 2-byte ESC sequence  (e.g. ESC 7, ESC 8, ESC c, ESC M …)
    return len(seq) >= 2

# ── stdin reader (background thread → asyncio queue) ──────────────────────

def stdin_reader() -> None:
    """Blocking stdin read loop.  Pushes each line into the asyncio queue.

    Runs in a daemon thread — when the sim exits stdin gets EOF, the thread
    finishes, and the server continues serving until shut down.

    Stdin is reopened with line-buffering so each newline-terminated line
    from the sim is delivered immediately rather than being held up by
    Python's default block-buffer for pipes/sockets.
    """
    try:
        sys.stdin = open(sys.stdin.fileno(), "r", buffering=1, closefd=False)
    except OSError:
        pass

    try:
        for line in sys.stdin:
            line = line.rstrip("\n")
            if line:
                asyncio.run_coroutine_threadsafe(stdin_queue.put(line), _loop)
    except Exception:
        pass

# ── broadcast task ────────────────────────────────────────────────────────

async def broadcast_from_stdin() -> None:
    """Consume lines from `stdin_queue`, batch consecutive UART bytes,
    record to the history ring-buffer, and fan out to all clients."""
    global _last_gpio

    while True:
        line = await stdin_queue.get()

        if line.startswith("uart:"):
            parts = [line[5:]]
        elif line.startswith("gpio:"):
            hex_val = line[5:]
            try:
                int(hex_val, 16)
            except ValueError:
                continue
            _last_gpio = hex_val
            msg = json.dumps({"type": "gpio", "data": hex_val})
            await _broadcast(msg)
            continue
        else:
            continue

        # drain immediately-available follow-up UART lines
        while True:
            try:
                nxt = stdin_queue.get_nowait()
            except asyncio.QueueEmpty:
                break
            if nxt.startswith("uart:"):
                parts.append(nxt[5:])
            elif nxt.startswith("gpio:"):
                # flush UART batch, then handle GPIO
                batched = "".join(parts)
                _record_uart_hex(batched)
                await _broadcast(json.dumps({"type": "uart", "data": batched}))
                hex_val = nxt[5:]
                try:
                    int(hex_val, 16)
                except ValueError:
                    parts = []
                    break
                _last_gpio = hex_val
                await _broadcast(json.dumps({"type": "gpio", "data": hex_val}))
                parts = []
                break
            else:
                pass  # skip unrecognised

        if parts:
            batched = "".join(parts)
            _record_uart_hex(batched)
            await _broadcast(json.dumps({"type": "uart", "data": batched}))


async def _broadcast(msg: str) -> None:
    """Send `msg` to every connected client concurrently."""
    if not clients:
        return

    async def _send_one(ws: "web.WebSocketResponse") -> None:
        try:
            await ws.send_str(msg)
        except Exception:
            clients.discard(ws)

    await asyncio.gather(*[_send_one(ws) for ws in list(clients)],
                         return_exceptions=True)

# ── WebSocket handler ─────────────────────────────────────────────────────

async def ws_handler(request: web.Request) -> "web.WebSocketResponse":
    ws = web.WebSocketResponse()
    await ws.prepare(request)

    # --- replay recent UART history to this client ---
    history_hex = _get_uart_history_hex()
    if history_hex:
        try:
            await ws.send_str(json.dumps({"type": "uart", "data": history_hex}))
        except Exception:
            return ws  # client already gone

    # --- send current GPIO state ---
    if _last_gpio is not None:
        try:
            await ws.send_str(json.dumps({"type": "gpio", "data": _last_gpio}))
        except Exception:
            return ws

    # --- add to broadcast set AFTER replaying history ---
    # this ensures the client gets the history first, then live data

    clients.add(ws)

    first_input = True  # prepend \\r on first keystroke to prime the TTY

    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                try:
                    data = json.loads(msg.data)
                except json.JSONDecodeError:
                    continue

                if data.get("type") == "uart_in":
                    hex_val = data.get("data", "")
                    try:
                        int(hex_val, 16)
                    except ValueError:
                        continue

                    if first_input:
                        first_input = False
                        try:
                            sys.stdout.write("in:0d\n")
                            sys.stdout.flush()
                            await asyncio.sleep(0.05)
                        except (BrokenPipeError, OSError):
                            pass

                    try:
                        for i in range(0, len(hex_val), 2):
                            byte_hex = hex_val[i : i + 2]
                            if len(byte_hex) == 2:
                                sys.stdout.write(f"in:{byte_hex}\n")
                                sys.stdout.flush()
                                await asyncio.sleep(0)
                    except (BrokenPipeError, OSError):
                        pass

            elif msg.type == web.WSMsgType.ERROR:
                pass

    finally:
        clients.discard(ws)
    return ws

# ── HTTP handlers ─────────────────────────────────────────────────────────

async def index(request: web.Request) -> "web.Response":
    html_path = os.path.join(os.path.dirname(__file__), "web_ui_v2.html")
    try:
        with open(html_path, encoding="utf-8") as fh:
            html = fh.read()
    except FileNotFoundError:
        return web.Response(text="web_ui_v2.html not found", status=500)
    return web.Response(text=html, content_type="text/html", charset="utf-8")

# ── main ──────────────────────────────────────────────────────────────────

def main() -> None:
    global _loop

    port = find_free_port(5000)
    sys.stderr.write(f"web_ui_v2: http://localhost:{port}\n")
    sys.stderr.flush()

    log_path = "/tmp/rvbucket_web_ui.log"
    try:
        log_fd = os.open(log_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        os.dup2(log_fd, 2)
        os.close(log_fd)
    except OSError:
        fd = os.open("/dev/null", os.O_WRONLY)
        os.dup2(fd, 2)
        os.close(fd)

    app = web.Application()
    app.router.add_get("/", index)
    app.router.add_get("/ws", ws_handler)

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    _loop = loop

    threading.Thread(target=stdin_reader, daemon=True).start()
    loop.create_task(broadcast_from_stdin())

    runner = web.AppRunner(app, access_log=None)
    loop.run_until_complete(runner.setup())
    site = web.TCPSite(runner, "0.0.0.0", port,
                       reuse_address=True, reuse_port=True)
    loop.run_until_complete(site.start())

    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    finally:
        loop.run_until_complete(runner.cleanup())
        loop.close()

if __name__ == "__main__":
    main()
