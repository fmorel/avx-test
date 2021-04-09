#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define DEBUG 0
#define MAX_START 10

static int n_tries = 0;

static int find_guess(int guess, int *start, int n_start);

/* Test 4 operations on the first two elements of start, n_start is >= 2*/
static int test_4op(int guess, const int *start, int n_start)
{
    int middle[MAX_START];
    int s0, s1;
    
    s0 = start[0];
    s1 = start[1];
    /* No interest in having 0 ... */
    if (s0 == 0 || s1 == 0)
        return 0;

#if DEBUG
    printf("Test %d %d\n", s0, s1);
#endif

    /* middle[0] will receive s0 op s1. Rest of array is unchanged */
    if (n_start > 2)
        memcpy(&middle[1], &start[2], (n_start - 2) * sizeof(int));

    /* + */
    middle[0] = s0 + s1;
    if (find_guess(guess, middle, n_start - 1)) {
        printf("%d + %d\n", s0, s1);
        return 1;
    }

    /* * */
    middle[0] = s0 * s1;
    if (find_guess(guess, middle, n_start - 1)) {
        printf("%d * %d\n", s0, s1);
        return 1;
    }

    /* - */
    if (s0 > s1) {
        middle[0] = s0 - s1;
        if (find_guess(guess, middle, n_start - 1)) {
            printf("%d - %d\n", s0, s1);
            return 1;
        }
    } else {
        middle[0] = s1 - s0;
        if (find_guess(guess, middle, n_start - 1)) {
            printf("%d - %d\n", s1, s0);
            return 1;
        }
    }

    /* / */
    if (s1 % s0 == 0) {
        middle[0] = s1 / s0;
        if (find_guess(guess, middle, n_start - 1)) {
            printf("%d / %d\n", s1, s0);
            return 1;
        }
    } else if (s0 % s1 == 0) {
         middle[0] = s0 / s1;
        if (find_guess(guess, middle, n_start - 1)) {
            printf("%d / %d\n", s0, s1);
            return 1;
        }
    }

    return 0;
}

static int find_guess(int guess, int *start, int n_start)
{
    int s0, s1, s_i, s_j, i, j;
    
    s0 = start[0];
    
    if (n_start == 1)
        n_tries++;

    /* Found it ! return 1 */
    if (s0 == guess)
        return 1;

    if (n_start == 1)
        return 0;   
    
    /* Only two elements, test 4 operations */
    if (n_start == 2)
        return test_4op(guess, start, n_start);
    
    s1 = start[1];
    /* Test by combining all the pairs by putting them in head of array */
    for (i = 0; i < n_start; i++) {
        s_i = start[i];
        start[i] = s0;
        start[0] = s_i;
        /* Test without s_i  */
        if (find_guess(guess, &start[1], n_start - 1)) {
            printf("%d unused\n", s_i);
            return 1;
        }
        for (j = i+1; j < n_start; j++) {
            s_j = start[j];
            start[j] = s1;
            start[1] = s_j;
            if (test_4op(guess, start, n_start))
                return 1;
            /* Put back array in place */
            start[j] = s_j;
        }
        start[i] = s_i;
    }
    start[0] = s0;
    start[1] = s1;
    return 0;
}

static void usage(void)
{
    printf("ok_count guess s1 s2 [s3 ...sN] : Find a combinaison of si numbers to find guess. N <= 10\n");
    exit(1);
}


int main(int argc, char **argv)
{
    int start[MAX_START];
    int guess, n_start;
    int i;

    if (argc < 4 || argc > 11)
        usage();

    guess = atoi(argv[1]);
    for (i = 2; i < argc; i++) {
        start[i - 2] = atoi(argv[i]);
    }
    n_start = argc - 2;

    if (!find_guess(guess, start, n_start))
        printf("No solution found \n");
    
    printf("%d tries\n", n_tries);
    return 0;
}

