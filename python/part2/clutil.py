#Authors:
#   Ian Johnson
#   Keith Brafford

import pyopencl as cl

import sys

class CLKernel(object):
    def __init__(self, filename, *args, **kwargs):
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
        
        self.gl_objects = []
        #TODO get these from kwargs
        self.kernelargs = None
        self.global_size = (0, )
        self.local_size = None
        self.PreExecute = None
        self.PostExecute = None
                
    def loadProgram(self, filename):
        #read in the OpenCL source file as a string
        f = open(filename, 'r')
        fstr = "".join(f.readlines())
        #print fstr
        #create the program
        self.program = cl.Program(self.ctx, fstr).build()

    def execute(self):
        if self.PreExecute:
            self.PreExecute()
            
        if self.gl_objects:
            cl.enqueue_acquire_gl_objects(self.queue, self.gl_objects)
            
        self.program.part2(self.queue, self.global_size, self.local_size,
                           *(self.kernelargs))

        if self.gl_objects:                            
            cl.enqueue_release_gl_objects(self.queue, self.gl_objects)
            
        self.queue.finish()
        
        if self.PostExecute:
            self.PostExecute()        

