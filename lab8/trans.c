/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
/* student ID : 522031910299 */
/* student Name : 陈启炜 */
/* 思路： ## Part B

对于32*32型转置，直接每次读取A的一行8个数字，填入B中相应位置。即将32*32分成几个8*8的小格，每个小格内实现转置即可。这里使用tmp[i]作为中间类似寄存器的作用，可以提高命中率。
对于61*67类似上述方法，进行及时break即可。
对于64*64，上述方法优化效果较差。查阅Log后发现，在B[k][m]的访问过程中经常会miss,sr不知道为什么，（现在知道了，刚好S=32，4行总共32个8int，因此8*8会冲突，选择4*4好很多但是还是没满分）但是每次访问四个B列，将读取到的数据，前四个填到对应的位置，后四个填到B[k][m+4]的位置，这里不会miss.这样就只需访问4个位置的B,接着访问左下角的A和B依次填入，最后处理右下角就行。
*/

#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    // improve the performance of the transpose function of the 32*32 matrix
    if (M == N && M == 32) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (k = i; k < i + 8; k++) {
                    tmp0 = A[k][j];
                    tmp1 = A[k][j + 1];
                    tmp2 = A[k][j + 2];
                    tmp3 = A[k][j + 3];
                    tmp4 = A[k][j + 4];
                    tmp5 = A[k][j + 5];
                    tmp6 = A[k][j + 6];
                    tmp7 = A[k][j + 7];
                    B[j][k] = tmp0;
                    B[j + 1][k] = tmp1;
                    B[j + 2][k] = tmp2;
                    B[j + 3][k] = tmp3;
                    B[j + 4][k] = tmp4;
                    B[j + 5][k] = tmp5;
                    B[j + 6][k] = tmp6;
                    B[j + 7][k] = tmp7;
                }
            }
        }
    }

    else if (M == N && M == 64) {
        // 思路：先将A的前4行转置到B的前4列，再将A的后4行转置到B的后4列，最后将B的后4列转置到B的前4行
        // 利用可以访问到的空间当cache，减少miss，although 读写次数增加，但是miss减少
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (k = i; k < i + 4; k++) {
                    tmp0 = A[k][j];
                    tmp1 = A[k][j + 1];
                    tmp2 = A[k][j + 2];
                    tmp3 = A[k][j + 3];
                    tmp4 = A[k][j + 4];
                    tmp5 = A[k][j + 5];
                    tmp6 = A[k][j + 6];
                    tmp7 = A[k][j + 7];
                    B[j][k] = tmp0;
                    B[j + 1][k] = tmp1;
                    B[j + 2][k] = tmp2;
                    B[j + 3][k] = tmp3;
                    B[j][k + 4] = tmp4;
                    B[j + 1][k + 4] = tmp5;
                    B[j + 2][k + 4] = tmp6;
                    B[j + 3][k + 4] = tmp7;
                }
                for (k = j; k < j + 4; k++) {
                    tmp0 = A[i + 4][k];
                    tmp1 = A[i + 5][k];
                    tmp2 = A[i + 6][k];
                    tmp3 = A[i + 7][k];
                    tmp4 = B[k][i + 4];
                    tmp5 = B[k][i + 5];
                    tmp6 = B[k][i + 6];
                    tmp7 = B[k][i + 7];
                    B[k][i + 4] = tmp0;
                    B[k][i + 5] = tmp1;
                    B[k][i + 6] = tmp2;
                    B[k][i + 7] = tmp3;
                    B[k + 4][i] = tmp4;
                    B[k + 4][i + 1] = tmp5;
                    B[k + 4][i + 2] = tmp6;
                    B[k + 4][i + 3] = tmp7;
                }
                for (k = i + 4; k < i + 8; k++) {
                    for (l = j + 4; l < j + 8; l++) {
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
    } else if (M == 61 && N == 67) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (k = i; k < i + 8 && k < N; k++) {
                    tmp0 = A[k][j];
                    tmp1 = A[k][j + 1];
                    tmp2 = A[k][j + 2];
                    tmp3 = A[k][j + 3];
                    tmp4 = A[k][j + 4];
                    if (j + 5 < 61)
                        tmp5 = A[k][j + 5];
                    if (j + 5 < 61)
                        tmp6 = A[k][j + 6];
                    if (j + 5 < 61)
                        tmp7 = A[k][j + 7];
                    B[j][k] = tmp0;
                    B[j + 1][k] = tmp1;
                    B[j + 2][k] = tmp2;
                    B[j + 3][k] = tmp3;
                    B[j + 4][k] = tmp4;
                    if (j + 5 < 61)
                        B[j + 5][k] = tmp5;
                    if (j + 5 < 61)
                        B[j + 6][k] = tmp6;
                    if (j + 5 < 61)
                        B[j + 7][k] = tmp7;
                }
            }
        }
    } else if (M == 61 && N == 168) {
        // 思路：先将A的前4行转置到B的前4列，再将A的后4行转置到B的后4列，最后将B的后4列转置到B的前4行
        // 利用可以访问到的空间当cache，减少miss，although 读写次数增加，但是miss减少
        for (i = 0; i < 64; i += 8) {
            for (j = 0; j < 56; j += 8) {
                for (k = i; k < i + 4; k++) {
                    tmp0 = A[k][j];
                    tmp1 = A[k][j + 1];
                    tmp2 = A[k][j + 2];
                    tmp3 = A[k][j + 3];
                    tmp4 = A[k][j + 4];
                    tmp5 = A[k][j + 5];
                    tmp6 = A[k][j + 6];
                    tmp7 = A[k][j + 7];
                    B[j][k] = tmp0;
                    B[j + 1][k] = tmp1;
                    B[j + 2][k] = tmp2;
                    B[j + 3][k] = tmp3;
                    B[j][k + 4] = tmp4;
                    B[j + 1][k + 4] = tmp5;
                    B[j + 2][k + 4] = tmp6;
                    B[j + 3][k + 4] = tmp7;
                }
                for (k = j; k < j + 4; k++) {
                    tmp0 = A[i + 4][k];
                    tmp1 = A[i + 5][k];
                    tmp2 = A[i + 6][k];
                    tmp3 = A[i + 7][k];
                    tmp4 = B[k][i + 4];
                    tmp5 = B[k][i + 5];
                    tmp6 = B[k][i + 6];
                    tmp7 = B[k][i + 7];
                    B[k][i + 4] = tmp0;
                    B[k][i + 5] = tmp1;
                    B[k][i + 6] = tmp2;
                    B[k][i + 7] = tmp3;
                    B[k + 4][i] = tmp4;
                    B[k + 4][i + 1] = tmp5;
                    B[k + 4][i + 2] = tmp6;
                    B[k + 4][i + 3] = tmp7;
                }
                for (k = i + 4; k < i + 8; k++) {
                    for (l = j + 4; l < j + 8; l++) {
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
        // for the ugly rectangle part
        i = 64;
        for (j = 0; j < 61; j++) {
            tmp0 = A[i][j];
            tmp1 = A[i + 1][j];
            tmp2 = A[i + 2][j];
            B[j][i] = tmp0;
            B[j][i + 1] = tmp1;
            B[j][i + 2] = tmp2;
        }

        i = 56;
        for (j = 0; j < 64; j++) {
            tmp0 = A[j][i];
            tmp1 = A[j][i + 1];
            tmp2 = A[j][i + 2];
            tmp3 = A[j][i + 3];
            tmp4 = A[j][i + 4];
            B[i][j] = tmp0;
            B[i + 1][j] = tmp1;
            B[i + 2][j] = tmp2;
            B[i + 3][j] = tmp3;
            B[i + 4][j] = tmp4;
        }
    } else {

        for (i = 0; i < N; i++) {
            for (j = 0; j < M; j++) {
                tmp0 = A[i][j];
                B[j][i] = tmp0;
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
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
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
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc);
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

// to test the trans-summit function
void test_transpose_submit()
{
    const int M = 64, N = 64;
    int A[N][M], B[M][N];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            A[i][j] = i + j;
        }
    }

    transpose_submit(M, N, A, B);
    if (is_transpose(M, N, A, B)) {
        printf("Transpose summit function is correct!\n");
    } else {
        printf("Transpose summit function is wrong!\n");
    }
}
/*
int main()
{
    test_transpose_submit();
    return 0;
}*/