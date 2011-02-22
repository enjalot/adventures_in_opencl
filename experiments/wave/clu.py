from OpenGL.GL import glFlush
import pyopencl as cl
import sys
import numpy

class CL:
    def __init__(self):
        plats = cl.get_platforms()
        from pyopencl.tools import get_gl_sharing_context_properties
        import sys 
        if sys.platform == "darwin":
            self.ctx = cl.Context(properties=get_gl_sharing_context_properties(),
                             devices=[])
        else:
            self.ctx = cl.Context(properties=[
                (cl.context_properties.PLATFORM, plats[0])]
                + get_gl_sharing_context_properties())
        self.queue = cl.CommandQueue(self.ctx)


    def loadProgram(self, filename):
        #read in the OpenCL source file as a string
        f = open(filename, 'r')
        fstr = "".join(f.readlines())
        #print fstr
        #create the program
        self.program = cl.Program(self.ctx, fstr).build()

    def loadData(self, pos_vbo, col_vbo, vel):
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


    def execute(self):
        #important to make a scalar arguement into a numpy scalar
        dt = numpy.float32(.01)
        cl.enqueue_acquire_gl_objects(self.queue, [self.pos_cl, self.col_cl])
        #2nd argument is global work size, 3rd is local work size, rest are kernel args
        self.program.wave(self.queue, self.pos.shape, None, 
                            self.pos_cl, 
                            self.col_cl, 
                            self.vel_cl, 
                            self.pos_gen_cl, 
                            self.vel_gen_cl, 
                            dt)
        cl.enqueue_release_gl_objects(self.queue, [self.pos_cl, self.col_cl])
        self.queue.finish()
        glFlush()
 
