#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define ORDER_MAX 3
const int order_mult[ORDER_MAX] = 
{
    1, 10, 100
};

const char decade_charsets[ORDER_MAX][3] = 
{
    {'I', 'V', 'X'}, /* 1, 5, 10 */
    {'X', 'L', 'C'}, /* 10, 50, 100 */
    {'C', 'D', 'M'}, /* 100, 500, 1000 */
};

/* Try to decode a bigram for 4 or 9 and also returns the order (0, 1 or 2 for respectively 1, 10, or 100) */
int bigram_decode(const char *buf, int idx, int *porder)
{
    int i;

    for (i = 0; i < ORDER_MAX; i++) {
        *porder = i;
        if (buf[idx] == decade_charsets[i][0]) {
            if (buf[idx+1] == decade_charsets[i][1]) {
                return 4 * order_mult[i];
            }
            if (buf[idx+1] == decade_charsets[i][2]) {
                return 9 * order_mult[i];
            }
        }
    }
    return 0;
}
/* Decode a singl roman character */
int monogram_decode(const char *buf, int idx, int *porder)
{
    int i;

    if (buf[idx] == 'M') {
        *porder = 3;
        return 1000;
    }

    for (i = 0; i < ORDER_MAX; i++) {
        *porder = i;
        if (buf[idx] == decade_charsets[i][0])
            return order_mult[i];
        if (buf[idx] == decade_charsets[i][1])
            return 5 * order_mult[i];
    }
    printf("Error, character %d is not a roman numeral\n", buf[idx]);
    return 0;
}
int decode(const char *buf) {
    
    int n, d, order, order_max, idx;
    n = 0;
    idx = 0;
    order_max = 4;
    /*Check if we start with a bigram */
    while (buf[idx]) {
        d = bigram_decode(buf, idx, &order);
        if (d) {
            if (order >= order_max) {
            wrong_order:
                printf("Error, roman numerals are not in expected order at idx %d\n",idx);
                return -1;
            }
            n += d;
            idx += 2;
            order_max = order;
        } else {
            d = monogram_decode(buf, idx, &order);
            if (order > order_max)
                goto wrong_order;
            if (!d)
                return -1;
            n += d;
            idx += 1;
            order_max = order;
        }
    }
    return n;
}
        

/* decade_charset contains roman character for one, five and ten of the current unit */
/* 0 <= d <= 9 */
void decimal_digit_encode(int d, char *buf, int *pidx, const char *decade_charset)
{
    int i;
    int idx = *pidx;
    if (d == 9) {
        buf[idx++] = decade_charset[0];
        buf[idx++] = decade_charset[2];
    } else if (d >= 5) {
        buf[idx++] = decade_charset[1];
        for (i = 5; i < d; i++) {
            buf[idx++] = decade_charset[0];
        }
    } else if (d == 4) {
        buf[idx++] = decade_charset[0];
        buf[idx++] = decade_charset[1];
    } else {
        for (i = 0; i < d; i++) {
            buf[idx++] = decade_charset[0];
        }
    }
    *pidx = idx;
}

int encode(int n, char *buf)
{
    int i, d, idx;

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
    d = n / order_mult[2];
    decimal_digit_encode(d, buf, &idx, decade_charsets[2]);
    n -= d * order_mult[2];
    d = n / order_mult[1];
    decimal_digit_encode(d, buf, &idx, decade_charsets[1]);
    n -= d * order_mult[1];
    decimal_digit_encode(n, buf, &idx, decade_charsets[0]);
    buf[idx] = '\0';
    return 0;
}
        
int main(int argc, char **argv)
{
    int n;
    char buf[128];

    if (argc <= 1) {
        printf("One argument expected\n");
        return -1;
    }

    if (isdigit(argv[1][0])) {
        n = atoi(argv[1]);
        if (encode(n, buf) < 0)
            return -1;
        printf("Decimal %d -> roman %s\n", n, buf);
    } else {
        n = decode(argv[1]);
        printf("Roman %s -> decimal %d\n", argv[1], n);
    }

    return 0;
}
