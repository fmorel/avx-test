#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "immintrin.h"

#include "openblas/cblas.h"

#define SIZE 2048

#define WATCHPOINT 16

void print_array(float *a, int len)
{
    int i;
    for (i=0; i < len ; i++) {
        printf("%.3f ", a[i]);
        if ((i%SIZE) == SIZE-1)
            printf("\n");
    }
    printf("\n");
}

void print_int_array(int *a, int len)
{
    int i;
    for (i=0; i < len ; i++) {
        printf("%d ", a[i]);
        if ((i%SIZE) == SIZE-1)
            printf("\n");
    }
}


typedef union Vec {
    __m256 v;
    __m256i vi;
    __m128 vs[2];
    float f[8];
    int i[8];
} Vec;



void old_mult(float (*a)[SIZE], float (*b)[SIZE], float (*c)[SIZE])
{
    int i,j,k;
   
    #pragma omp parallel for
    for (j = 0; j < SIZE; j++) {
        float bt[SIZE];
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
    print_array(&c[WATCHPOINT][WATCHPOINT], 64);
}



void new_mult(float (*a)[SIZE], float (*b)[SIZE], float (*c)[SIZE])
{
    int i,j,k;

    #pragma omp parallel for
    for (j = 0; j < SIZE; j++) {
        float bt[SIZE];
        for (k = 0; k < SIZE ; k++) {
            bt[k] = b[k][j];
        }
        for (i = 0; i < SIZE; i++) {
            c[i][j] = 0;
            for (k = 0; k < SIZE; k+=8) {
                __m256 av, bv;
                Vec cv;
                av = _mm256_load_ps(&a[i][k]);
                bv = _mm256_load_ps(&bt[k]);
                cv.v = _mm256_dp_ps(av, bv, 0xF1);
                
                c[i][j] += cv.f[0] + cv.f[4];
            }
        }
    }
    printf("NEW : ");
    print_array(&c[WATCHPOINT][WATCHPOINT], 64);
}



// Use FMA instead of dot products
void mult_block8(Vec *a, Vec *b, Vec *c)
{
    Vec a_const[8][8];
    int i, j;
    for (i = 0; i < 8; i++) {
        for (j=0; j < 8; j++) {
            a_const[i][j].v = _mm256_set1_ps(a[i].f[j]);
        }
    }

    for (i = 0; i < 8; i++) {
        for (j=0; j < 8; j++) {
            c[i].v = _mm256_fmadd_ps(a_const[i][j].v,  b[j].v, c[i].v);
        }
    }
}

void new_mult2(const float (*a)[SIZE], const float (*b)[SIZE], float (*c)[SIZE])
{
    Vec idx[8];
    int j_block, i, k;

    //Store block offset for A and transposed subblock for B
    for (i = 0; i < 8; i++) {
        for (k = 0; k < 8 ; k++) {
            idx[i].i[k] = i * SIZE + k;
        }
    }

    #pragma omp parallel for
    for (j_block = 0; j_block < SIZE; j_block+=8) {
        Vec a_block[8], b_block[SIZE], c_block[8];
        int i_block, k_block, i;
        for (i = 0; i < SIZE; i++) {
            b_block[i].v = _mm256_i32gather_ps(&b[(i/8)*8][j_block], idx[i%8].vi, 4);
        }

        for (i_block = 0; i_block < SIZE; i_block+=8) {
            //Clear destination block
            for (i = 0; i < 8; i++) {
                c_block[i].v = _mm256_set1_ps(0.0);
            }
            //Compute block
            for (k_block = 0; k_block < SIZE; k_block+=8) {
                //Extract A subblock
                for (i = 0; i < 8; i++) {
                    a_block[i].v = _mm256_i32gather_ps(&a[i_block][k_block], idx[i].vi, 4);
                }
                mult_block8(a_block, &b_block[k_block], c_block);
            }

            //Store block 
            for (i = 0; i < 8; i++) {
                _mm256_store_ps(&c[i_block + i][j_block], c_block[i].v);
            }
        }
    }
    printf("BLOCK : ");
    print_array(&c[WATCHPOINT][WATCHPOINT], 64);
}

int main(int argc, char **argv) {
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


    if (argc <= 1 || atoi(argv[1]) == 0) {
        printf("Run old mult\n");
        old_mult(a,b,c);
    } else {
        switch (atoi(argv[1])) {
        case 1:
            printf("Run simd mult\n");
            new_mult(a,b,c);
            break;
        case 2:
            printf("Run simd block mult\n");
            new_mult2(a,b,c);
            break;
        case 3:
            printf("Open BLAS mult\n");
            cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                SIZE, SIZE, SIZE, 1.0, 
                &a[0][0], SIZE, 
                &b[0][0], SIZE,
                0, &c[0][0], SIZE);
    
            printf("LIB : ");
            print_array(&c[WATCHPOINT][WATCHPOINT], 64);
            break;
        }
    }
    
    return 0;
}
