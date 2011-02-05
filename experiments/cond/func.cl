


//__kernel void func_kernel(__global float* a, __global float* b, __global float* c, __constant struct Params* params)
__kernel void func_kernel(  __global float* a,
                            __global float* b,
                            __global float* c
                            )
{
    unsigned int i = get_global_id(0);

    int j = 1;
    int iej = i != j;
    c[i] = 0.1 * iej;
}
