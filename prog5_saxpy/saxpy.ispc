
export void saxpy_ispc(uniform int N,
                       uniform float scale,
                            uniform float X[],
                            uniform float Y[],
                            uniform float result[])
{
    // uniform int u=N/8;
    // #pragma GCC unroll 8
    foreach (i = 0 ... N) { 
    // for (uniform int i=0; i<N; i+=programCount){
    // for(int j=1;j<=10;j++)    
    // result[i] = scale / X[i] + Y[i];
    // streaming_store(result+i+programIndex,scale*X[programIndex+i]+Y[programIndex+i]);  giveup in ISPC 我用AVX写好了
    result[i]=scale*X[i]+Y[i];
    }
    // foreach (i = 0 ... u) { 
    //     result[8*i] = scale * X[8*i] + Y[8*i];
    //     result[8*i+1] = scale * X[8*i+1] + Y[8*i+1];
    //     result[8*i+2] = scale * X[8*i+2] + Y[8*i+2];
    //     result[8*i+3] = scale * X[8*i+3] + Y[8*i+3];
    //     result[8*i+4] = scale * X[8*i+4] + Y[8*i+4];
    //     result[8*i+5] = scale * X[8*i+5] + Y[8*i+5];
    //     result[8*i+6] = scale * X[8*i+6] + Y[8*i+6];
    //     result[8*i+7] = scale * X[8*i+7] + Y[8*i+7];
    // } 无法这样写优化
    // for (uniform int i=0; i<N; i+=programCount)
    // {
    //     result[programIndex+i] = scale*X[programIndex+i] + Y[programIndex+i];  //说明ISPC已经优化完循环展开了 
    // }
}
task void saxpy_ispc_task(uniform int N,
                               uniform int span,
                               uniform float scale,
                               uniform float X[], 
                               uniform float Y[],
                               uniform float result[])
{

    uniform int indexStart = taskIndex * span;
    uniform int indexEnd = min(N, indexStart + span);

    foreach (i = indexStart ... indexEnd) {
                // for(int j=1;j<=10;j++)        
        // result[i] = scale / X[i] + Y[i];
        result[i] = scale * X[i] + Y[i];
    }
}

export void saxpy_ispc_withtasks(uniform int N,
                               uniform float scale,
                               uniform float X[],
                               uniform float Y[],
                               uniform float result[])
{

    uniform int span = N / 64;  // 64 tasks

    launch[N/span] saxpy_ispc_task(N, span, scale, X, Y, result);
}
