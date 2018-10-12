#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "immintrin.h"

#define SIZE 4096

void print_array(float *a, int len)
{
    int i;
    for (i=0; i < len ; i++) {
        printf("%.3f ", a[i]);
    }
    printf("\n");
}

union Vec {
    __m256 v;
    float f[8];
};



void old_mult(float (*a)[SIZE], float (*b)[SIZE], float (*c)[SIZE])
{
    int i,j,k;
    float bt[SIZE];
   
    for (j = 0; j < SIZE; j++) {
        for (k = 0; k < SIZE ; k++) {
            bt[k] = b[k][j];
        }
        for (i = 0; i < SIZE; i++) {
            c[i][j] = 0;
            for (k = 0; k < SIZE; k++) {
                c[i][j] += a[i][k] * bt[k];
            }
        }
    }

    printf("OLD : ");
    print_array(&c[128][120], 16);
}



void new_mult(float (*a)[SIZE], float (*b)[SIZE], float (*c)[SIZE])
{
    float bt[SIZE];
    int i,j,k;

    #pragma omp parallel for
    for (j = 0; j < SIZE; j++) {
        for (k = 0; k < SIZE ; k++) {
            bt[k] = b[k][j];
        }
        for (i = 0; i < SIZE; i++) {
            c[i][j] = 0;
            for (k = 0; k < SIZE; k+=8) {
                __m256 av, bv;
                union Vec cv;
                av = _mm256_load_ps(&a[i][k]);
                bv = _mm256_load_ps(&bt[k]);
                cv.v = _mm256_dp_ps(av, bv, 0xF1);
                
                c[i][j] += cv.f[0] + cv.f[4];
            }
        }
    }
    printf("NEW : ");
    print_array(&c[128][120], 16);
}


int main(void) {
    static float a[SIZE][SIZE];
    static float b[SIZE][SIZE];
    static float c[SIZE][SIZE];

    int i,j,k;


    for (i=0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            a[i][j] = (float)(i*SIZE + j) / (SIZE*SIZE);
            b[i][j] = 1.0 - a[i][j];
        }
    }
    print_array (&a[128][128], 8);
    print_array (&b[128][128], 8);

    //old_mult(a,b,c);
    new_mult(a,b,c);
    
    return 0;
}
