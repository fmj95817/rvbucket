#include <stdio.h>
#include <ctype.h>

#define BUF_SIZE 128

int main(void)
{
    printf("calc> ");

    char buf[BUF_SIZE];
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        printf("ERR: no input\n");
        return 1;
    }

    /* evaluate left-to-right (no operator precedence) */
    int result = 0;
    char op = '+';
    const char *p = buf;

    while (*p) {
        /* skip whitespace */
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (!*p) break;

        /* parse number */
        if (!isdigit((unsigned char)*p)) {
            printf("ERR: expected number at '%s'\n", p);
            return 1;
        }
        int val = 0;
        while (isdigit((unsigned char)*p)) {
            val = val * 10 + (*p - '0');
            p++;
        }

        /* apply operator */
        switch (op) {
        case '+':
            result += val;
            break;
        case '-':
            result -= val;
            break;
        case '*':
            result *= val;
            break;
        case '/':
            if (val == 0) {
                printf("ERR: divide by zero\n");
                return 1;
            }
            result /= val;
            break;
        default:
            printf("ERR: unknown op '%c'\n", op);
            return 1;
        }

        /* skip whitespace, get next operator */
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p) {
            op = *p++;
            if (op != '+' && op != '-' && op != '*' && op != '/') {
                printf("ERR: unknown operator '%c'\n", op);
                return 1;
            }
        }
    }

    printf("= %d\n", result);
    return 0;
}
