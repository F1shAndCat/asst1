#include <stdio.h>
#include <algorithm>
#include <pthread.h>
#include <math.h>
#include <immintrin.h>
#include "CycleTimer.h"
#include "sqrt_ispc.h"

using namespace ispc;

extern void sqrtSerial(int N, float startGuess, float* values, float* output);

static void verifyResult(int N, float* result, float* gold) {
    for (int i=0; i<N; i++) {
        if (fabs(result[i] - gold[i]) > 1e-4) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}
// {

//     static const float kThreshold = 0.00001f;

//     for (int i=0; i<N; i++) {

//         float x = values[i];
//         float guess = initialGuess;

//         float error = fabs(guess * guess * x - 1.f);

//         while (error > kThreshold) {
//             guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
//             error = fabs(guess * guess * x - 1.f);
//         }

//         output[i] = x * guess;
//     }
// }


static void sqrt_AVX(int N,float initialGuess,float* values,float* output)
{
    static const float kThreshold = 0.00001f;
    __m256 ones=_mm256_set1_ps(1.f);
    __m256 thresholds=_mm256_set1_ps(kThreshold);
    __m256 threes=_mm256_set1_ps(3.f);
    __m256 halfs=_mm256_set1_ps(0.5f);
    unsigned int* mask2bool=new unsigned int[8];
    for(int i=0;i<N;i+=8)
    {
        __m256 x=_mm256_setr_ps(values[i],values[i+1],values[i+2],values[i+3],values[i+4],values[i+5],values[i+6],values[i+7]);
        __m256 guess=_mm256_set1_ps(initialGuess);
        __m256 error=_mm256_mul_ps(x,guess);
        __m256 tmp1,tmp2;
        bool flag;
        error=_mm256_mul_ps(error,guess);
        error=_mm256_sub_ps(error,ones);
        // error=(__m256)_mm256_abs_epi32((__m256i)error);
        error=_mm256_and_ps(error, _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF))); 
        tmp1=_mm256_cmp_ps(error,thresholds,0x1e);
        _mm256_storeu_ps((float *)mask2bool,tmp1);
        flag=false;
        for(int j=0;j<8;j++)
        flag|=(mask2bool[j]==0xFFFFFFFF);
        // int cnt=30;
        // while(!(_mm256_testc_ps(_mm256_cmp_ps(error,thresholds,0x1e),zeros)))
        // while(cnt--)
        while(flag)
        {
            tmp1=_mm256_mul_ps(guess,threes);
            tmp2=_mm256_mul_ps(x,guess);
            tmp2=_mm256_mul_ps(tmp2,guess);
            tmp2=_mm256_mul_ps(tmp2,guess);
            tmp1=_mm256_sub_ps(tmp1,tmp2);
            guess=_mm256_mul_ps(tmp1,halfs);
            error=_mm256_mul_ps(x,guess);
            error=_mm256_mul_ps(error,guess);
            error=_mm256_sub_ps(error,ones);
            error=_mm256_and_ps(error, _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF))); //浮点数只要改变符号位数,与补码运算不同.
            // error=_mm256_castsi256_ps(_mm256_abs_epi32((__m256i)error)); 没用 强制转换和castsi256起到的效果相同
            tmp1=_mm256_cmp_ps(error,thresholds,0x1e);
            _mm256_storeu_ps((float*)mask2bool,tmp1);
            flag=false;
            for(int j=0;j<8;j++)
            flag|=(mask2bool[j]==0xFFFFFFFF);
        }
        x=_mm256_mul_ps(x,guess);
        _mm256_storeu_ps(output+i,x); //架构是64位 所以得用unaligned版本的
    }
}
int main() {

    const unsigned int N = 20 * 1000 * 1000;
    const float initialGuess = 1.0f;

    float* values = new float[N];
    float* output = new float[N];
    float* gold = new float[N];

    for (unsigned int i=0; i<N; i++)
    {
        // TODO: CS149 students.  Attempt to change the values in the
        // array here to meet the instructions in the handout: we want
        // to you generate best and worse-case speedups
        
        // starter code populates array with random input values
        values[i] = .001f + 2.998f * static_cast<float>(rand()) / RAND_MAX;
    }

    // generate a gold version to check results
    for (unsigned int i=0; i<N; i++)
        gold[i] = sqrt(values[i]);

    //
    // And run the serial implementation 3 times, again reporting the
    // minimum time.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrtSerial(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

    printf("[sqrt serial]:\t\t[%.3f] ms\n", minSerial * 1000);

    verifyResult(N, output, gold);

    //
    // Compute the image using the ispc implementation; report the minimum
    // time of three runs.
    //
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc(N, initialGuess, values, output);
        // sqrt_AVX(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    printf("[sqrt ispc]:\t\t[%.3f] ms\n", minISPC * 1000);

    verifyResult(N, output, gold);

    // My AVX/AVX2 implementation
    double minAVX = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        // sqrt_ispc(N, initialGuess, values, output);
        sqrt_AVX(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minAVX = std::min(minAVX, endTime - startTime);
    }

    printf("[sqrt avx]:\t\t[%.3f] ms\n", minAVX * 1000);

    verifyResult(N, output, gold);
    // Clear out the buffer
    for (unsigned int i = 0; i < N; ++i)
        output[i] = 0;

    //
    // Tasking version of the ISPC code
    //
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc_withtasks(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    printf("[sqrt task ispc]:\t[%.3f] ms\n", minTaskISPC * 1000);

    verifyResult(N, output, gold);

    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
    printf("\t\t\t\t(%.2fx speedup from AVX implementation)\n", minSerial/minAVX);
    printf("\t\t\t\t(%.2fx is the ISPC/AVX speedup ratio)\n", minISPC/minAVX);
    printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);

    delete [] values;
    delete [] output;
    delete [] gold;

    return 0;
}
