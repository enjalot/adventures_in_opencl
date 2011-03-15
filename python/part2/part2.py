from OpenGL.GL import GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, glFlush
from OpenGL.arrays import vbo

import clutil
import numpy

class Part2CL(clutil.CLKernel):
    def __init__(self, num, dt, *args, **kwargs):
        #setup initial values of arrays
        clutil.CLKernel.__init__(self, *args, **kwargs)
        self.num = num
        self.dt = numpy.float32(dt)
        pos = numpy.ndarray((num, 4), dtype=numpy.float32)
        col = numpy.ndarray((num, 4), dtype=numpy.float32)
        vel = numpy.ndarray((num, 4), dtype=numpy.float32)

        from math import sqrt, sin, cos
        import random
        random.seed()
        for i in xrange(0, num):
            rad = random.uniform(.2, .5);
            x = rad*sin(2*3.14 * i/num)
            z = 0.
            y = rad*cos(2*3.14 * i/num)

            pos[i,0] = x
            pos[i,1] = y
            pos[i,2] = z
            pos[i,3] = 1.

            col[i,0] = 1.
            col[i,1] = 0.
            col[i,2] = 0.
            col[i,3] = 1.

            life = random.random()
            vel[i,0] = x*2.
            vel[i,1] = y*2.
            vel[i,2] = 3.
            vel[i,3] = life

        #print pos
        #print col
        #print vel

        #for some reason trying to do this inside CL.loadData gives me errors on mac
        pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
        pos_vbo.bind()
        col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
        col_vbo.bind()
        
        self.loadData(pos_vbo, col_vbo, vel)
         


    def loadData(self, pos_vbo, col_vbo, vel):
        import pyopencl as cl
        mf = cl.mem_flags
        self.pos_vbo = pos_vbo
        self.col_vbo = col_vbo

        self.pos = pos_vbo.data
        self.col = col_vbo.data
        self.vel = vel

        #Setup vertex buffer objects and share them with OpenCL as GLBuffers
        self.pos_vbo.bind()
        self.pos_cl = cl.GLBuffer(self.ctx, mf.READ_WRITE, int(self.pos_vbo.buffers[0]))
        self.col_vbo.bind()
        self.col_cl = cl.GLBuffer(self.ctx, mf.READ_WRITE, int(self.col_vbo.buffers[0]))

        #pure OpenCL arrays
        self.vel_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=vel)
        self.pos_gen_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=self.pos)
        self.vel_gen_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=self.vel)
        self.queue.finish()

        # set up the list of GL objects to share with opencl
        self.gl_objects = [self.pos_cl, self.col_cl]
        
        # set up the Kernel argument list
        self.kernelargs = (self.pos_cl, 
                           self.col_cl, 
                           self.vel_cl, 
                           self.pos_gen_cl, 
                           self.vel_gen_cl, 
                           self.dt)



