#include <stdio.h>
#include <stdlib.h>

char decade_charsets[3][3] = 
{
    {'I', 'V', 'X'}, /* 1, 5, 10 */
    {'X', 'L', 'C'}, /* 10, 50, 100 */
    {'C', 'D', 'M'}, /* 100, 500, 1000 */
};

/* decade_charset contains roman character for one, five and ten of the current unit */
/* 0 <= n <= 9 */
void decimal_encode(int n, char *buf, int *pidx, const char *decade_charset)
{
    int i;
    int idx = *pidx;
    if (n == 9) {
        buf[idx++] = decade_charset[0];
        buf[idx++] = decade_charset[2];
    } else if (n >= 5) {
        buf[idx++] = decade_charset[1];
        for (i = 5; i < n; i++) {
            buf[idx++] = decade_charset[0];
        }
    } else if (n == 4) {
        buf[idx++] = decade_charset[0];
        buf[idx++] = decade_charset[1];
    } else {
        for (i = 0; i < n; i++) {
            buf[idx++] = decade_charset[0];
        }
    }
    *pidx = idx;
}
        
int main(int argc, char **argv)
{
    int i, n, d, idx;
    char buf[128];

    if (argc <= 1) {
        printf("One argument expected\n");
        return -1;
    }
    n = atoi(argv[1]);
    if (n > 5000) {
        printf("n is too big (max is 5000)\n");
        return -1;
    }
    d = n / 1000;
    idx = 0;
    for (i = 0; i < d; i++) {
        buf[idx++] = 'M';
    }
    n-= d * 1000;
    d = n / 100;
    decimal_encode(d, buf, &idx, decade_charsets[2]);
    n -= d * 100;
    d = n / 10;
    decimal_encode(d, buf, &idx, decade_charsets[1]);
    n -= d * 10;
    decimal_encode(n, buf, &idx, decade_charsets[0]);
    buf[idx] = '\0';

    printf("Number is : %s\n", buf);
    return 0;
}
