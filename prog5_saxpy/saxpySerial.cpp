
void saxpySerial(int N,
                       float scale,
                       float X[],
                       float Y[],
                       float result[])
{

    for (int i=0; i<N; i++) {
        // for(int j=1;j<=10;j++)
        // result[i] = scale / X[i] + Y[i];
        result[i] = scale * X[i] + Y[i];
    }
}

