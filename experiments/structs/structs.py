#http://documen.tician.de/pyopencl/

import pyopencl as cl
import numpy as np
import struct

ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags

prg = cl.Program(ctx, """
    typedef struct Test
    {
        float A;
        float B;
        int C;
        float x;
    } Test;

    __kernel void sum(__global const float *a,
                      __global const float *b, 
                      __global float *c,
                      //__constant int test
                      __constant struct Test* test
                      )
    {
      int gid = get_global_id(0);
      c[gid] = test->A * a[gid] + test->B * b[gid] + test->C;
      //c[test] = 666.;
    }
    """).build()

def add(a, b):
    a_buf = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=a)
    b_buf = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=b)
    dest_buf = cl.Buffer(ctx, mf.WRITE_ONLY, b.nbytes)

    test1 = struct.pack('ffif', .5, 10., 4, 1.)
    #test2 = struct.pack("i", 4)
    print test1, len(test1)
    
    global_size = a.shape
    local_size = None
    prg.sum(queue, global_size, local_size, a_buf, b_buf, dest_buf, test1)

    c = np.empty_like(a)
    cl.enqueue_read_buffer(queue, dest_buf, c).wait()
    return c

if __name__ == "__main__":
    x = np.ndarray((10, 1), dtype=np.float32)
    y = np.ndarray((10, 1), dtype=np.float32)
    x[:] = 1.
    y[:] = 2.

    z = add(x,y)
    print z
