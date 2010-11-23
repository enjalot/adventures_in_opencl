
#define NEGATIVE
#include "inc.cl"


__kernel void b_kernel(__global float* a, __global float* b, __global float* c, __constant struct Params* params)
{
    unsigned int i = get_global_id(0);

    c[i] = params->c1 * a[i] + params->c2 * b[i];
    c[i] *= def_sign; //sign is defined in lvl2.cl depending if POSITIVE or NEGATIVE is defined
}
