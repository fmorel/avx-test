#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "immintrin.h"

#include "openblas/cblas.h"

#define SIZE 2048


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
    print_array(&c[0][0], 64);
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
                Vec cv;
                av = _mm256_load_ps(&a[i][k]);
                bv = _mm256_load_ps(&bt[k]);
                cv.v = _mm256_dp_ps(av, bv, 0xF1);
                
                c[i][j] += cv.f[0] + cv.f[4];
            }
        }
    }
    printf("NEW : ");
    print_array(&c[16][16], 64);
}


// Consider B is transposed so the multiplication is just dot products

void clear_block8(float *c)
{
    int i,j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            c[i*SIZE + j] = 0;
        }
    }
}

void mult_block8(Vec *a, Vec *b, Vec *c)
{
    Vec tmp, tmp2;
    int i, j;
    for (i = 0; i < 8; i++) {
        tmp.v = _mm256_dp_ps(a[i].v, b[0].v, 0xF1);
        tmp2.v = _mm256_dp_ps(a[i].v, b[4].v, 0xF1);
        tmp.v = _mm256_add_ps(tmp.v, _mm256_dp_ps(a[i].v, b[1].v, 0xF2));
        tmp2.v = _mm256_add_ps(tmp2.v, _mm256_dp_ps(a[i].v, b[5].v, 0xF2));
        tmp.v = _mm256_add_ps(tmp.v, _mm256_dp_ps(a[i].v, b[2].v, 0xF4));
        tmp2.v = _mm256_add_ps(tmp2.v, _mm256_dp_ps(a[i].v, b[6].v, 0xF4));
        tmp.v = _mm256_add_ps(tmp.v,_mm256_dp_ps(a[i].v, b[3].v, 0xF8));
        tmp2.v = _mm256_add_ps(tmp2.v, _mm256_dp_ps(a[i].v, b[7].v, 0xF8));

        tmp.vs[0] = _mm_add_ps(tmp.vs[0], tmp.vs[1]);
        tmp.vs[1] = _mm_add_ps(tmp2.vs[0], tmp2.vs[1]);

        c[i].v = _mm256_add_ps(c[i].v, tmp.v);
    }

}

void new_mult2(const float (*a)[SIZE], const float (*b)[SIZE], float (*c)[SIZE])
{
    Vec a_idx[8], b_idx[8];
    int j_block, i, k;

    //Store block offset for A and transposed subblock for B
    for (i = 0; i < 8; i++) {
        for (k = 0; k < 8 ; k++) {
            a_idx[i].i[k] = i * SIZE + k;
            b_idx[i].i[k] = k * SIZE + i;
        }
    }

    #pragma omp parallel for
    for (j_block = 0; j_block < SIZE; j_block+=8) {
        Vec a_block[8], b_block[SIZE], c_block[8];
        int i_block, k_block, i;
        //Prefetch B blocks to avoid cache penalties
        for (i = 0; i < SIZE; i++) {
            b_block[i].v = _mm256_i32gather_ps(&b[(i/8)*8][j_block], b_idx[i%8].vi, 4);
        }

        for (i_block = 0; i_block < SIZE; i_block+=8) {
            //Clear idestination block
            for (i = 0; i < 8; i++) {
                c_block[i].v = _mm256_set1_ps(0.0);
            }
            //Compute block
            for (k_block = 0; k_block < SIZE; k_block+=8) {
                //Extract A subblock
                for (i = 0; i < 8; i++) {
                    a_block[i].v = _mm256_i32gather_ps(&a[i_block][k_block], a_idx[i].vi, 4);
                }
                mult_block8(a_block, &b_block[k_block], c_block);
            }

            //Store block 
            for (i = 0; i < 8; i++) {
                _mm256_store_ps(&c[i_block + i][j_block], c_block[i].v);
            }
        }
    }
    printf("NEW2 : ");
    print_array(&c[16][16], 64);
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
    print_array (&a[0][0], 64);
    print_array (&b[0][0], 64);

    //printf("Run old mult\n");
    //old_mult(a,b,c);
    
    //printf("Run new mult\n");
    //new_mult(a,b,c);
    
    printf("Run new mult2\n");
    new_mult2(a,b,c);
    
#ifdef  CBLAS
    printf("Open BLAS mult\n");
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                SIZE, SIZE, SIZE, 1.0, 
                &a[0][0], SIZE, 
                &b[0][0], SIZE,
                0, &c[0][0], SIZE);
    
    printf("LIB : ");
    print_array(&c[16][16], 64);
#endif
    
    return 0;
}
