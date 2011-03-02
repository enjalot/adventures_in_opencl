import pyopencl as cl
print dir(cl)
a = cl.array.arange(400, dtype=numpy.float32)
b = cl.array.arange(400, dtype=numpy.float32)

krnl = ReductionKernel(ctx, numpy.float32, neutral="0",
                reduce_expr="a+b", map_expr="x[i]*y[i]",
                        arguments="__global float *x, __global float *y")

my_dot_prod = krnl(a, b).get()
print my_dot_prod


