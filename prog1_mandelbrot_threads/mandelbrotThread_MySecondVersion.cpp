#include <stdio.h>
#include <thread>
#include <algorithm>
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


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

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
    int cur=0;
    float xx,yy; unsigned int x,y;
    for(yy=args->y0,y=0;y<args->height;y++,yy+=args->y1)
    {
    for(xx=args->x0,x=0;x<args->width;x++,xx+=args->x1)
    {
        // xx=x*args->x1+args->x0;
        // yy=y*args->y1+args->y0; 乘法也没用 就是浮点误差.
        args->output[cur]=mandel(xx,yy,args->maxIterations);
        cur++;
    }
    }
    // int cnt= std::min((args->height/args->numThreads),args->height-args->threadId*((args->height/args->numThreads)));
    // mandelbrotSerial(args->x0,args->y0,args->x1,args->y1,args->width,args->height,args->threadId*((args->height/args->numThreads)),cnt,args->maxIterations,args->output);
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
    float xstep=(x1-x0)/width;
    float ystep=(y1-y0)/height;
    static constexpr int MAX_THREADS = 64;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }
    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {  //把X_1,Y_1改为步长即可同步.
      
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].height = (i+1)*height/numThreads-i*height/numThreads;
        args[i].x0 = x0;
        args[i].y0 = y0+(i*height/numThreads)*ystep;
        args[i].x1 = xstep;
        args[i].y1 = ystep;
        args[i].width = width; 
        // args[i].y1 = y0+(i+1)*(y1-y0)/numThreads;  //浮点除,精确
        // args[i].height = (i+1)*height/numThreads-i*height/numThreads; //整除,是不精确的. 所以精确-不精确会出来一个不精确的图,就是有畸变,不能这么只整除一个的划分
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output+(int)(i*height/numThreads)*width; //精确 ok
        //args[i].output=output;
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

