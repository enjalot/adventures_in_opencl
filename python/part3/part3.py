#http://documen.tician.de/pyopencl/

import pyopencl as cl
import numpy as np
import struct

ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags

f = open("part3.cl", 'r')
fstr = "".join(f.readlines())
#print fstr
#create the program
prg = cl.Program(ctx, fstr).build()


def add(a, b):
    a_buf = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=a)
    b_buf = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=b)
    dest_buf = cl.Buffer(ctx, mf.WRITE_ONLY, b.nbytes)

    test1 = struct.pack('ffffi', .5, 10., 0., 0., 3)
    print test1, len(test1), struct.calcsize('ffffi')

    test_buf = cl.Buffer(ctx, mf.READ_ONLY, len(test1))
    cl.enqueue_write_buffer(queue, test_buf, test1).wait()
    
    global_size = a.shape
    local_size = None
    prg.part3(queue, global_size, local_size, a_buf, b_buf, dest_buf, test_buf)
    queue.finish()

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
