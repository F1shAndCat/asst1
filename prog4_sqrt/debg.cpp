#include <bits/stdc++.h>
using namespace std;
int main()
{
    float* a=(float *)malloc(sizeof(float)*20000000);
    float* arrayY = new float[20000000];
    for(int i=0;i<20000000;i++)
    a[i]=i;
    free(a);
}