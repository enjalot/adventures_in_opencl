#Authors: Ian Johnson, Keith Brafford

from OpenGL.GL import GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, glFlush

import pyopencl as cl

import sys
import numpy

class CLKernel(object):
    def __init__(self, filename):
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

        self.loadProgram(filename)
        
        self.kernelargs = None
        self.gl_objects = []
        self.PreExecute = None
        self.PostExecute = None
                
    def loadProgram(self, filename):
        #read in the OpenCL source file as a string
        f = open(filename, 'r')
        fstr = "".join(f.readlines())
        print fstr
        #create the program
        self.program = cl.Program(self.ctx, fstr).build()

    def execute(self):
        if self.PreExecute:
            self.PreExecute()
            
        if self.gl_objects:
            cl.enqueue_acquire_gl_objects(self.queue, self.gl_objects)
            
        #self.program.part2(self.queue, self.pos.shape, None, 
        #                   *(self.kernelargs))

        if self.gl_objects:                            
            cl.enqueue_release_gl_objects(self.queue, self.gl_objects)
            
        self.queue.finish()
        glFlush()
        
        if self.PostExecute:
            self.PostExecute()        

