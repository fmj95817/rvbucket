#include <stdio.h>

int main()
{
    int n = 0;
    for (int i = 1; i <= 100; i++) {
        n += i;
    }
    printf("hello, world %d!\n", n);
    return 0;
}