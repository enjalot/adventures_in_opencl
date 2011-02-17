from OpenGL.GL import *
from OpenGL.GLU import *
#from OpenGL.GLUT import *
#from OpenGL.raw.GL.VERSION.GL_1_5 import glBufferData as rawGlBufferData
#from OpenGL import platform, GLX, WGL 
from OpenGL.arrays import vbo

import pyopencl as cl

import sys
import numpy

class CL:
    def __init__(self):
        plats = cl.get_platforms()
        ctx_props = cl.context_properties

        props = [(ctx_props.PLATFORM, plats[0])] 

        import sys 
        if sys.platform == "linux2":
            props.append(
                    (ctx_props.GL_CONTEXT_KHR, platform.GetCurrentContext()))
            props.append(
                    (ctx_props.GLX_DISPLAY_KHR, 
                        GLX.glXGetCurrentDisplay()))
        elif sys.platform == "win32":
            props.append(
                    (ctx_props.GL_CONTEXT_KHR, platform.GetCurrentContext()))
            props.append(
                    (ctx_props.WGL_HDC_KHR, 
                        WGL.wglGetCurrentDC()))
        elif sys.platform == "darwin":
            pass
        else:
            raise NotImplementedError("platform '%s' not yet supported" 
                    % sys.platform)

        self.ctx = cl.Context(properties=props)
        self.queue = cl.CommandQueue(self.ctx)

    

    def loadProgram(self, filename):
        #read in the OpenCL source file as a string
        f = open(filename, 'r')
        fstr = "".join(f.readlines())
        print fstr
        #create the program
        self.program = cl.Program(self.ctx, fstr).build()

    def loadData(self, pos, col, vel):
        mf = cl.mem_flags
        self.pos = pos
        self.col = col
        self.vel = vel

        #Setup vertex buffer objects and share them with OpenCL as GLBuffers
        self.pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
        self.pos_vbo.bind()
        self.pos_cl = cl.GLBuffer(self.ctx, mf.READ_WRITE, int(self.pos_vbo.buffers[0]))

        self.col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
        self.col_vbo.bind()
        self.col_cl = cl.GLBuffer(self.ctx, mf.READ_WRITE, int(self.col_vbo.buffers[0]))

        #pure OpenCL arrays
        self.vel_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=vel)
        self.pos_gen_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=pos)
        self.vel_gen_cl = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=vel)
        self.queue.finish()


    def execute(self):
        #important to make a scalar arguement into a numpy scalar
        dt = numpy.float32(.01)
        cl.enqueue_acquire_gl_objects(self.queue, [self.pos_cl, self.col_cl])
        #2nd argument is global work size, 3rd is local work size, rest are kernel args
        self.program.part2(self.queue, self.pos.shape, None, 
                            self.pos_cl, 
                            self.col_cl, 
                            self.vel_cl, 
                            self.pos_gen_cl, 
                            self.vel_gen_cl, 
                            dt)
        cl.enqueue_release_gl_objects(self.queue, [self.pos_cl, self.col_cl])
        self.queue.finish()
        glFlush()
        

