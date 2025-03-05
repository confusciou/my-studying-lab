/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_d_0[] = "transpose_0";
void transpose_0(int M, int N, int A[N][M], int B[M][N])
{
    for(int i = 0 ; i < M; i+=8){
        for(int j = 0; j < N; j+=8){
            for(int s = 0; s < 8; s++){
                for(int k = 0; k < 8; k++){
                    B[i+s][j+k] = A[i+s][j+k];
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
  

}
char transpose_submit_02[] = "Transpose 02";
void transpose_02(int M, int N, int A[N][M], int B[M][N]){
    //s=5 E=1，b=5
    // int nss = N/5;
    // int nleast = N - nss*5;
    int mss = M/5;
    int tmp;
    for(int mttt = 0; mttt < 5; mttt++){
        for(int col = 0; col < N; col++){
        //行号n = col
            for(int m = mttt*5,mL = mttt*5+5; m < mL; m++){
                tmp = A[col][m];
                B[m][col] = tmp;
            }
        }
    }
    for(int col = 0; col < N; col++){
        for(int mlea = mss*5; mlea < M; mlea++){
       
            tmp = A[col][mlea];
            B[mlea][col] = tmp;
        }
    }
    if(is_transpose( M,  N,  A,  B) == 0 ){
        printf("???????");
    }
}
/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_0, transpose_d_0); 
     

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    registerTransFunction(transpose_02, transpose_submit_02);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

