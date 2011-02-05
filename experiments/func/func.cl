#include "inc.cl"

float function_example(float a, float b)
{
    return a + b;
}

__kernel void func_kernel(  __global float* a, 
                            __global float* b, 
                            __global float* c, 
                            __constant struct Params* params
                            )
{
    unsigned int i = get_global_id(0);

    c[i] = function_example(a[i], b[i]);
}
