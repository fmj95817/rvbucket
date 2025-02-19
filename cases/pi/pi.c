#include <stdio.h>

int main()
{
    float pi = 0;
    for (int i = 0; i < 100; i++) {
        pi += (((i % 2) ? -1.0f : 1.0f) / ((float)(2 * i + 1)));
    }

    printf("pi = %f\n", pi * 4.0f);
    return 0;
}
