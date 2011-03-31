#from OpenGL.GL import GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, glFlush, glGenBuffers, glBindBuffer
from OpenGL.GL import *
from OpenGL.arrays import vbo

import numpy as np
import struct
import pyopencl as cl

from initialize import timings       

class ChaoticArtist(object):
    def __init__(self, ntracers, dt=.01, maxlife=1.0, dlife=.01, beta=.5, A=.1, B=.1, F=48., oomph=.01):
        self.clinit()
        self.prgs = {}  #store our programs
        self.loadProgram("cartist.cl")
        #self.loadProgram("update.cl")
        
        self.ntracers = ntracers

        self.dt = np.float32(dt)
        self.maxlife = np.float32(maxlife)
        self.dlife = np.float32(dlife)
        beta = np.float32(beta)
        A = np.float32(A)
        B = np.float32(B)
        F = np.float32(F)

        #self.params = struct.pack('fffffff', self.dt, self.maxlife, self.dlife, beta, A, B, np.pi)

        params = [self.dt, self.maxlife, self.dlife, beta, A, B, F, np.pi, oomph]
        self.params = np.array(params, dtype=np.float32)
        #print params
        #print self.params
         
        self.count = 0
    
    @timings
    def execute(self, newp, t):
        self.count += 1
        if self.count >= self.ntracers-1:
            self.count = 0
       
        cl.enqueue_acquire_gl_objects(self.queue, self.gl_objects)

        global_size = (self.ntracers,)
        local_size = None

                
        ca_kernelargs = (self.pos_cl, 
                      self.col_cl, 
                      self.time_cl, 
                      self.props_cl, 
                      np.int32(self.count),
                      np.float32(t),
                      newp,
                      self.params_cl
                      )
                       
        #print len(self.params)
        self.prgs["cartist"].cartist(self.queue, global_size, local_size, *(ca_kernelargs))

        cl.enqueue_release_gl_objects(self.queue, self.gl_objects)
        self.queue.finish()
 


    def loadData(self, pos_vbo, col_vbo, time, props):
        import pyopencl as cl
        mf = cl.mem_flags
        self.pos_vbo = pos_vbo
        self.col_vbo = col_vbo

        self.pos = pos_vbo.data
        self.col = col_vbo.data
        self.time = time
        self.props = props

        #Setup vertex buffer objects and share them with OpenCL as GLBuffers
        self.pos_vbo.bind()
        self.pos_cl = cl.GLBuffer(self.ctx, mf.READ_WRITE, int(self.pos_vbo.buffers[0]))

        self.col_vbo.bind()
        self.col_cl = cl.GLBuffer(self.ctx, mf.READ_WRITE, int(self.col_vbo.buffers[0]))

        #pure OpenCL arrays
        self.time_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=self.time)
        self.props_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=self.props)
        self.params_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=self.params)
        self.queue.finish()

        # set up the list of GL objects to share with opencl
        self.gl_objects = [self.pos_cl, self.col_cl]
 
 
    def clinit(self):
        plats = cl.get_platforms()
        from pyopencl.tools import get_gl_sharing_context_properties
        import sys 
        if sys.platform == "darwin":
            self.ctx = cl.Context(properties=get_gl_sharing_context_properties(),
                             devices=[])
        else:
            self.ctx = cl.Context(properties=[
                (cl.context_properties.PLATFORM, plats[0])]
                + get_gl_sharing_context_properties(), devices=None)
                
        self.queue = cl.CommandQueue(self.ctx)

    def loadProgram(self, filename):
        #read in the OpenCL source file as a string
        f = open(filename, 'r')
        fstr = "".join(f.readlines())
        #print fstr
        #create the program
        prg_name = filename.split(".")[0]   #e.g. wave from wave.cl
        self.prgs[prg_name] = cl.Program(self.ctx, fstr).build()



    def render(self):


        #glColor3f(1,0,0)
        glEnable(GL_POINT_SMOOTH)
        glPointSize(5)

        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        glDisable(GL_DEPTH_TEST)

        """
        glColor3f(1., 0, 0)
        glBegin(GL_POINTS)
        for p in self.pos_vbo.data:
            glVertex3f(p[0], p[1], p[2])

        glEnd()
        """

        self.col_vbo.bind()
        glColorPointer(4, GL_FLOAT, 0, self.col_vbo)

        self.pos_vbo.bind()
        glVertexPointer(4, GL_FLOAT, 0, self.pos_vbo)

        glEnableClientState(GL_VERTEX_ARRAY)
        glEnableClientState(GL_COLOR_ARRAY)
        glDrawArrays(GL_POINTS, 0, self.ntracers)

        glDisableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_VERTEX_ARRAY)

        glDisable(GL_BLEND)
        #glDisable(GL_DEPTH_TEST)
        glEnable(GL_DEPTH_TEST)

