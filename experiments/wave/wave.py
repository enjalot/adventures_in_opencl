#from OpenGL.GL import GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, glFlush, glGenBuffers, glBindBuffer
from OpenGL.GL import *
from OpenGL.arrays import vbo

import numpy
from math import sqrt, sin, cos
import random

from clu import CL
       

class Wave:
    def __init__(self):

        #set up initial conditions
        #need to setup VBOs before starting CL context
        self.initial_conditions()

        self.cl = CL()
        self.cl.loadProgram("wave.cl")


        self.cl.loadData(self.pos_vbo, self.col_vbo, self.vel)

    def initial_conditions(self):
        num = 20000
        #setup initial values of arrays
        pos = numpy.ndarray((num, 4), dtype=numpy.float32)
        col = numpy.ndarray((num, 4), dtype=numpy.float32)
        vel = numpy.ndarray((num, 4), dtype=numpy.float32)

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

        self.pos = pos
        self.vel = vel
        self.col = col

        self.pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
        self.pos_vbo.bind()
        self.col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
        self.col_vbo.bind()
        
        self.num = len(self.pos)
         


    def execute(self):
        self.cl.execute()


    def render(self):

        #glColor3f(1,0,0)
        glEnable(GL_POINT_SMOOTH)
        glPointSize(5)

        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

        self.cl.col_vbo.bind()
        glColorPointer(4, GL_FLOAT, 0, self.cl.col_vbo)

        self.cl.pos_vbo.bind()
        glVertexPointer(4, GL_FLOAT, 0, self.cl.pos_vbo)

        glEnableClientState(GL_VERTEX_ARRAY)
        glEnableClientState(GL_COLOR_ARRAY)
        glDrawArrays(GL_POINTS, 0, self.num)

        glDisableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_VERTEX_ARRAY)

        glDisable(GL_BLEND)

