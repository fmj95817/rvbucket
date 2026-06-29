#include <stdint.h>
#include <stdio.h>
#include "drivers/uart/uart.h"

#define PLIC_BASE_ADDR     0x31100000u
#define PLIC_CTX_BASE      0x200000u
#define PLIC_CLAIM         (PLIC_CTX_BASE + 0x04u)

#define MCAUSE_INTERRUPT_BIT    0x80000000u
#define MCAUSE_M_EXT            11u

#define UART_IRQ_BIT            (1u << UART_IRQ_NUM)
#define EXPR_BUF_SIZE           128u

static volatile char expr_buf[EXPR_BUF_SIZE];
static volatile uint32_t expr_len;
static volatile uint32_t expr_done;
static volatile uint32_t expr_overflow;
static volatile uint32_t uart_irq_count;
static volatile uint32_t bad_trap_cause;

static void uart_puts(const char *str)
{
    while (*str != '\0') {
        uart_write(*str++);
    }
}

typedef struct parser {
    const volatile char *buf;
    uint32_t len;
    uint32_t pos;
    const char *err;
} parser_t;

static uint32_t plic_claim_irq(void)
{
    return *(volatile uint32_t *)(PLIC_BASE_ADDR + PLIC_CLAIM);
}

static void plic_complete_irq(uint32_t irq)
{
    *(volatile uint32_t *)(PLIC_BASE_ADDR + PLIC_CLAIM) = irq;
}

uint32_t trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mtval;

    if (mcause != (MCAUSE_INTERRUPT_BIT | MCAUSE_M_EXT)) {
        bad_trap_cause = mcause;
        return mepc;
    }

    uint32_t irq = plic_claim_irq();
    if (irq != UART_IRQ_NUM) {
        plic_complete_irq(irq);
        return mepc;
    }

    uart_irq_count++;
    while (uart_status() & UART_STS_RX_VALID) {
        char ch = uart_read();

        if (ch == '\b' || ch == 0x7f) {
            if (expr_len != 0u) {
                expr_len--;
                uart_puts("\b \b");
            }
            continue;
        }

        if (ch == '\r' || ch == '\n') {
            uart_puts("\r\n");
            expr_done = 1;
            break;
        }

        if (expr_len < (EXPR_BUF_SIZE - 1u)) {
            expr_buf[expr_len++] = ch;
            uart_write(ch);
        } else {
            expr_overflow = 1;
        }
    }

    plic_complete_irq(irq);
    return mepc;
}

static int is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

static char parser_peek(parser_t *p)
{
    while (p->pos < p->len && is_space(p->buf[p->pos])) {
        p->pos++;
    }

    if (p->pos >= p->len) {
        return '\0';
    }
    return p->buf[p->pos];
}

static int32_t parse_expr(parser_t *p);

static int32_t parse_factor(parser_t *p)
{
    char ch = parser_peek(p);

    if (ch == '+') {
        p->pos++;
        return parse_factor(p);
    }

    if (ch == '-') {
        p->pos++;
        return -parse_factor(p);
    }

    if (ch == '(') {
        p->pos++;
        int32_t val = parse_expr(p);
        if (p->err != NULL) {
            return 0;
        }
        if (parser_peek(p) != ')') {
            p->err = "missing ')'";
            return 0;
        }
        p->pos++;
        return val;
    }

    if (ch < '0' || ch > '9') {
        p->err = "expected number";
        return 0;
    }

    int32_t val = 0;
    while (p->pos < p->len) {
        ch = p->buf[p->pos];
        if (ch < '0' || ch > '9') {
            break;
        }
        val = val * 10 + (ch - '0');
        p->pos++;
    }
    return val;
}

static int32_t parse_term(parser_t *p)
{
    int32_t val = parse_factor(p);

    while (p->err == NULL) {
        char op = parser_peek(p);
        if (op != '*' && op != '/') {
            break;
        }
        p->pos++;

        int32_t rhs = parse_factor(p);
        if (p->err != NULL) {
            return 0;
        }

        if (op == '*') {
            val *= rhs;
        } else {
            if (rhs == 0) {
                p->err = "divide by zero";
                return 0;
            }
            val /= rhs;
        }
    }

    return val;
}

static int32_t parse_expr(parser_t *p)
{
    int32_t val = parse_term(p);

    while (p->err == NULL) {
        char op = parser_peek(p);
        if (op != '+' && op != '-') {
            break;
        }
        p->pos++;

        int32_t rhs = parse_term(p);
        if (p->err != NULL) {
            return 0;
        }

        if (op == '+') {
            val += rhs;
        } else {
            val -= rhs;
        }
    }

    return val;
}

static int parse_line(int32_t *result, const char **err)
{
    parser_t p = {
        .buf = expr_buf,
        .len = expr_len,
        .pos = 0,
        .err = NULL
    };

    *result = parse_expr(&p);
    if (p.err != NULL) {
        *err = p.err;
        return 0;
    }

    if (parser_peek(&p) != '\0') {
        *err = "trailing characters";
        return 0;
    }

    *err = NULL;
    return 1;
}

int main(void)
{
    expr_len = 0;
    expr_done = 0;
    expr_overflow = 0;
    uart_irq_count = 0;
    bad_trap_cause = 0;

    uart_puts("uart_irq_calc> ");

    while (!expr_done && !expr_overflow && bad_trap_cause == 0u) {
        __asm__ volatile ("wfi");
    }

    plic_complete_irq(UART_IRQ_NUM);

    if (bad_trap_cause != 0u) {
        printf("ERR: bad trap cause 0x%08x\n", (unsigned int)bad_trap_cause);
        return 1;
    }

    if (expr_overflow) {
        printf("ERR: expression too long\n");
        return 1;
    }

    expr_buf[expr_len] = '\0';

    int32_t result = 0;
    const char *err = NULL;
    if (!parse_line(&result, &err)) {
        printf("ERR: %s\n", err);
        return 1;
    }

    printf("= %d\n", (int)result);
    printf("uart_irq_count=%u\n", (unsigned int)uart_irq_count);
    return 0;
}
