import pyopencl
from pyopencl.array import arange, Array
from pyopencl.reduction import ReductionKernel
import numpy

ctx = pyopencl.create_some_context()
queue = pyopencl.CommandQueue(ctx)

#print dir(cl)
#a = arange(queue, 400, dtype=numpy.float32)
#b = arange(queue, 400, dtype=numpy.float32)
acpu = numpy.zeros((100, 1), dtype=numpy.int32)
for i in xrange(0,100):
    if i % 5 == 0:
        acpu[i] = 1

a = Array(queue, (100,1), numpy.int32)
a.set(acpu)
queue.finish()

krnl = ReductionKernel(ctx, numpy.int32, neutral="0",
                reduce_expr="a+b", map_expr="x[i]", #*y[i]",
                        arguments="__global int *x")#, __global in *y")

my_sum = krnl(a).get()
queue.finish()
print my_sum


