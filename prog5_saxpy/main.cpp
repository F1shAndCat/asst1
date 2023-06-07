#include <stdio.h>
#include <algorithm>
#include <immintrin.h>
#include <cstdlib>
#include <thread>
#include "CycleTimer.h"
#include "saxpy_ispc.h"
extern void saxpySerial(int N, float a, float* X, float* Y, float* result);

std::thread cnt[65];
// return GB/s
static float
toBW(int bytes, float sec) {
    return static_cast<float>(bytes) / (1024. * 1024. * 1024.) / sec;
}

static float
toGFLOPS(int ops, float sec) {
    return static_cast<float>(ops) / 1e9 / sec;
}

static void verifyResult(int N, float* result, float* gold) {
    for (int i=0; i<N; i++) {
        if (result[i] != gold[i]) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}
static void saxpyAVX(int N, float a, float* X, float* Y, float* result)
{
    __m256 Av=_mm256_set1_ps(a);
    for(int i=0;i<N;i+=8)
    {
        __m256 Xv=_mm256_setr_ps(X[i],X[i+1],X[i+2],X[i+3],X[i+4],X[i+5],X[i+6],X[i+7]);
        __m256 Yv=_mm256_setr_ps(Y[i],Y[i+1],Y[i+2],Y[i+3],Y[i+4],Y[i+5],Y[i+6],Y[i+7]);
        _mm256_stream_ps(result+i,_mm256_add_ps(_mm256_mul_ps(Xv,Av),Yv));
    }
}
static void saxpyAVX_Task(int N,int threads,float a, float* X, float* Y, float* result)
{
    N/=threads;
    int cur=0;
    saxpyAVX(N,a,X,Y,result);
    for(int i=1;i<threads;i++)
    {
        cur+=N;
        cnt[i]=std::thread(saxpyAVX,N,a,X+cur,Y+cur,result+cur);
    }
    for(int i=1;i<threads;i++)
    cnt[i].join();
}
using namespace ispc;


int main() {

    const unsigned int N = 20 * 1000 * 1000; // 20 M element vectors (~80 MB)
    const unsigned int TOTAL_BYTES = 4 * N * sizeof(float);  //X,Y,Result,i?
    const unsigned int TOTAL_FLOPS = 2 * N;
    const int Threads=32;
    float scale = 2.f;
    //char * x=new char;
    float* arrayX =(float*)std::aligned_alloc(32, N*sizeof(float));
    float* arrayY =(float*)std::aligned_alloc(32, N*sizeof(float));
    float* resultSerial =(float*)std::aligned_alloc(32, N*sizeof(float));
    float* resultISPC =(float*)std::aligned_alloc(32, N*sizeof(float));
    float* resultAVX =(float*)std::aligned_alloc(32, N*sizeof(float));
    float* resultTasks =(float*)std::aligned_alloc(32, N*sizeof(float));
    // float* arrayY = new float[N];
    // float* resultSerial = new float[N];
    // float* resultISPC = new float[N];
    // float* resultTasks = new float[N];
    // initialize array values
    for (unsigned int i=0; i<N; i++)
    {
        arrayX[i] = i;
        arrayY[i] = i;
        resultSerial[i] = 0.f;
        resultISPC[i] = 0.f;
        resultTasks[i] = 0.f;
    }

    //
    // Run the serial implementation. Repeat three times for robust
    // timing.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime =CycleTimer::currentSeconds();
         saxpySerial(N, scale, arrayX, arrayY, resultSerial);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

// printf("[saxpy serial]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
//           minSerial * 1000,
//           toBW(TOTAL_BYTES, minSerial),
//           toGFLOPS(TOTAL_FLOPS, minSerial));

    
//     Run the ISPC (single core) implementation
    
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc(N, scale, arrayX, arrayY, resultISPC);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }
    verifyResult(N, resultISPC, resultSerial);
        printf("[saxpy ispc]:\t\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minISPC * 1000,
           toBW(TOTAL_BYTES, minISPC),
           toGFLOPS(TOTAL_FLOPS, minISPC));

    double minAVX = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpyAVX(N, scale, arrayX, arrayY, resultAVX);
        double endTime = CycleTimer::currentSeconds();
        minAVX = std::min(minAVX, endTime - startTime);
    }

    verifyResult(N, resultAVX, resultSerial);

    printf("[saxpy free_cache_avx]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minAVX * 1000,
           toBW(TOTAL_BYTES, minAVX),
           toGFLOPS(TOTAL_FLOPS, minAVX));
    // Run the ISPC (multi-core) implementation
    
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc_withtasks(N, scale, arrayX, arrayY, resultTasks);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    verifyResult(N, resultTasks, resultSerial);

    printf("[saxpy task ispc]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minTaskISPC * 1000,
           toBW(TOTAL_BYTES, minTaskISPC),
           toGFLOPS(TOTAL_FLOPS, minTaskISPC));
    double minTaskAVX = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpyAVX_Task(N,Threads, scale, arrayX, arrayY, resultAVX);
        double endTime = CycleTimer::currentSeconds();
        minTaskAVX = std::min(minTaskAVX, endTime - startTime);
    }
    verifyResult(N, resultAVX, resultSerial);

    printf("[saxpy freeche_tasks_avx]:\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minTaskAVX * 1000,
           toBW(TOTAL_BYTES, minTaskAVX),
           toGFLOPS(TOTAL_FLOPS, minTaskAVX));
    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minISPC/minTaskAVX);
    printf("\t\t\t\t(%.2fx speedup from free of one cache acess)\n", minAVX/minTaskAVX);
    printf("\t\t\t\t(%.2fx speedup from use of tasks)\n", minTaskISPC/minTaskAVX);
    // printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
    // printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);

    free(arrayX);
    free(arrayY);
    free(resultSerial);
    free(resultISPC);
    free(resultTasks);

    return 0;
}
