#include <stdio.h>
#include <thread>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


// extern void mandelbrotSerial(
//     float x0, float y0, float x1, float y1,
//     int width, int height,
//     int startRow, int numRows,
//     int maxIterations,
//     int output[]);

extern int mandel(float c_re, float c_im, int count) //单纯复数的平方 也可以z_i=z_{i-1}^{2}+C (C是复数)
;
//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs * const args) {

    // TODO FOR CS149 STUDENTS: Implement the body of the worker
    // thread here. Each thread should make a call to mandelbrotSerial()
    // to compute a part of the output image.  For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.
    int cnt=0;
    float xstep=(args->x1-args->x0)/args->width;
    float ystep=(args->y1-args->y0)/args->height;
    float xx,yy; unsigned int x,y;
    for(yy=args->y0,y=0;y<args->height;y++,yy+=ystep)
    for(xx=args->x0,x=0;x<args->width;x++,xx+=xstep)
    {
        args->output[cnt]=mandel(xx,yy,args->maxIterations);
        cnt++;
    }
    printf("Hello world from thread %d\n", args->threadId);   //先把乘优化成加先
} 

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 64;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {
      
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0+i*(y1-y0)/numThreads;
        args[i].x1 = x1;
        args[i].y1 = y0+(i+1)*(y1-y0)/numThreads;
        args[i].width = width;
        args[i].height = (i+1)*height/numThreads-i*height/numThreads;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output+i*height/numThreads*width;
      
        args[i].threadId = i;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i=1; i<numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }
    
    workerThreadStart(&args[0]);

    // join worker threads
    for (int i=1; i<numThreads; i++) {
        workers[i].join();
    }
}

