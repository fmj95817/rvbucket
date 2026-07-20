# SD-SPI Packet Interface

The SD-SPI controller and physical adapter use two ordered, backpressured
packet streams. Only one command may be outstanding.

## Command stream

`sdspi_cmd` is driven by the controller.

- `CONFIG` updates `enable` and `clock_div`. The controller sends pending
  configuration before the next command. `clock_div` is the number of system
  clock cycles per SPI clock period; zero selects the adapter reset default.
- `RESET` aborts the active protocol operation, releases chip select, and
  returns the adapter to its initialization state.
- `COMMAND` starts one SD command. Command metadata remains valid in this
  packet only; it is not repeated on data packets.
- `WRITE_DATA` transfers one 32-bit word after a successful write-command
  response. `last` marks the final word of the complete request.

## Data stream

`sdspi_data` is driven by the physical adapter.

- `STATUS` reports card presence, write protection, and the latest R1 idle
  state. The adapter sends an initial status after reset and another status
  after a card-state change.
- `RESPONSE` is sent after the complete command response has been received.
  For R1B it is delayed until card busy is released.
- `READ_DATA` transfers one 32-bit word after `RESPONSE`. `last` marks the
  final word of the complete request.
- `WRITE_DONE` is sent after the final data-response token and card busy
  release. Every accepted command terminates with either `RESPONSE` alone,
  `RESPONSE` plus read data, or `RESPONSE` plus `WRITE_DONE`.

For both data directions, bits `[7:0]` contain the byte transferred first on
the SPI wire, followed by `[15:8]`, `[23:16]`, and `[31:24]`. The current
protocol subset requires data lengths to be multiples of four bytes.

The adapter may pause SCLK between bytes while either interface is
backpressured. CMD18 stops after the requested block count and is terminated
by a subsequent CMD12. CMD25 emits one multi-block write token per block and
automatically emits the SPI stop-transmission token after the final block.

## DMA completion ordering

For card-to-memory transfers, the controller waits for the final AXI write
response, then reads the complete destination range back with the DMA
transaction ID. The read data is discarded. Command and data completion are
reported only after this visibility drain, so a CPU using another AXI ID cannot
observe completion before the DMA writes are visible. Covering the full range
also prevents completion from depending on one write-buffer-forwarded word.

The memory subsystem must not reorder the drain reads ahead of preceding DMA
writes. The Xilinx FPGA target therefore configures MIG for strict request
ordering; the C model memory path is already strongly ordered.
