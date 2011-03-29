typedef struct Params
{
    float A;
    float B;
    float x[2];  //padding
    int C;
} Params;


__kernel void part3(__global const float *a,
                  __global const float *b, 
                  __global float *c,
                  __constant struct Params* test
                  )
{
    int gid = get_global_id(0);
    c[gid] = test->A * a[gid] + test->B * b[gid] + test->C;
}


