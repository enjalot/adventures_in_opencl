#define STRINGIFY(A) #A

std::string kernel_source = STRINGIFY(

#include "inc.cl"

__kernel void a_kernel(__global float* a, __global float* b, __global float* c, __constant struct Params params)
{
    unsigned int i = get_global_id(0);

    c[i] = params->c1 * a[i] + params->c2 * b[i];
}
);
