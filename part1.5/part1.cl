#define STRINGIFY(A) #A

std::string kernel_source = STRINGIFY(
__kernel void part1(__global float* a, __global float* b, __global float* c)
{
    unsigned int i = get_global_id(0);

    c[i] = a[i] + b[i];
}
);
