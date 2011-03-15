#from OpenGL.GL import GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, glFlush
from OpenGL.GL import *
from OpenGL.GLU import *


import clutil
import numpy

class Part2CL(clutil.CLKernel):
    def __init__(self, num, dt, *args, **kwargs):
        #setup initial values of arrays
        clutil.CLKernel.__init__(self, *args, **kwargs)
        self.num = num
        self.dt = numpy.float32(dt)


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

        self.global_size = (self.num,)
        print self.global_size


    def render(self):
        
        glEnable(GL_POINT_SMOOTH)
        glPointSize(2)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

        #setup the VBOs
        self.col_vbo.bind()
        glColorPointer(4, GL_FLOAT, 0, self.col_vbo)

        self.pos_vbo.bind()
        glVertexPointer(4, GL_FLOAT, 0, self.pos_vbo)

        glEnableClientState(GL_VERTEX_ARRAY)
        glEnableClientState(GL_COLOR_ARRAY)
        #draw the VBOs
        glDrawArrays(GL_POINTS, 0, self.num)

        glDisableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_VERTEX_ARRAY)

        glDisable(GL_BLEND)
     

