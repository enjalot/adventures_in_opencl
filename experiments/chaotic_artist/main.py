#basic glut setup learned from here:
#http://www.java2s.com/Open-Source/Python/Game-2D-3D/PyOpenGL/PyOpenGL-Demo-3.0.1b1/PyOpenGL-Demo/NeHe/lesson2.py.htm

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import sys

#helper modules
import glutil
from vector import Vec

#OpenCL code
import cartist 
#functions for initial values of particles
import initialize
import numpy
from math import sin, cos

ntracers = 50000
dt = .01
dlife = .00005
x = 0.
y = 0.

sub_intervals = 10

class window(object):
    def __init__(self, *args, **kwargs):
        #mouse handling for transforming scene
        self.mouse_down = False
        self.mouse_old = Vec([0., 0.])
        self.rotate = Vec([-85., 0., 80.])
        self.translate = Vec([0., 0., 0.])
        self.initrans = Vec([0., -1., -8.])

        self.width = 640
        self.height = 480

        glutInit(sys.argv)
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
        glutInitWindowSize(self.width, self.height)
        glutInitWindowPosition(0, 0)
        self.win = glutCreateWindow("Chaotic Artist")

        #gets called by GLUT every frame
        glutDisplayFunc(self.draw)

        #handle user input
        glutKeyboardFunc(self.on_key)
        glutMouseFunc(self.on_click)
        glutMotionFunc(self.on_mouse_motion)
        
        #this will call draw every 30 ms
        glutTimerFunc(30, self.timer, 30)

        #setup OpenGL scene
        self.glinit()

        #set up initial conditions
        (pos_vbo, col_vbo, time, props) = initialize.ca(ntracers)
        #create our OpenCL instance
        #self.cle = part2.Part2(num, dt, "part2.cl")
        self.cle = cartist.ChaoticArtist(ntracers, dt=dt, dlife=dlife)
        self.cle.loadData(pos_vbo, col_vbo, time, props)

        self.t = 0.
        self.x = 1.
        self.y = 1.
        self.z = 0.
        #newp = numpy.array([.25, .25, 0., 1.], dtype=numpy.float32)
        #print "newp", newp
        #self.cle.execute(newp, self.t) 
        glutMainLoop()
        

    def update(self):
        self.t += dt
        #self.x += y*dt + sin(self.t)*dt
        #self.y += x*dt + cos(self.t)*dt
        a = .3
        b = .2
        c = 18
        dx = -self.y - self.z
        dy = self.x + a * self.y
        dz = b + self.z * (self.x - c)
        self.x += dx * dt
        self.y += dy * dt
        self.z += dz * dt
        newp = numpy.array([self.x, self.y, self.z, 1.], dtype=numpy.float32)
        self.cle.execute(newp, self.t)

    def draw(self):
        """Render the particles"""        
        #TODO: 
        # * set up Ortho2D viewing and mouse controls
        # * find better color mapping for height
        for i in xrange(0, sub_intervals):
            self.update()
        
        #update or particle positions by calling the OpenCL kernel
        #self.cle.execute(subintervals) 
        glFlush()

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        #handle mouse transformations
        glTranslatef(self.initrans.x, self.initrans.y, self.initrans.z)
        glRotatef(self.rotate.x, 1, 0, 0)
        glRotatef(self.rotate.y, 0, 1, 0) #we switched around the axis so make this rotate_z
        glRotatef(self.rotate.z, 0, 0, 1) #we switched around the axis so make this rotate_z
        glTranslatef(self.translate.x, self.translate.y, self.translate.z)
        
        glScalef(.1,.1,.1)
        #render the particles
        self.cle.render()

        #draw the x, y and z axis as lines
        glutil.draw_axes()

        glutSwapBuffers()


    def glinit(self):
        glViewport(0, 0, self.width, self.height)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(60., self.width / float(self.height), .1, 1000.)
        glMatrixMode(GL_MODELVIEW)


    ###GL CALLBACKS
    def timer(self, t):
        glutTimerFunc(t, self.timer, t)
        glutPostRedisplay()

    def on_key(self, *args):
        ESCAPE = '\033'
        if args[0] == ESCAPE or args[0] == 'q':
            sys.exit()
        elif args[0] == 't':
            print initialize.timings

    def on_click(self, button, state, x, y):
        if state == GLUT_DOWN:
            self.mouse_down = True
            self.button = button
        else:
            self.mouse_down = False
        self.mouse_old.x = x
        self.mouse_old.y = y

    
    def on_mouse_motion(self, x, y):
        dx = x - self.mouse_old.x
        dy = y - self.mouse_old.y
        if self.mouse_down and self.button == 0: #left button
            self.rotate.x += dy * .2
            self.rotate.y += dx * .2
        elif self.mouse_down and self.button == 2: #right button
            self.translate.z -= dy * .01 
        self.mouse_old.x = x
        self.mouse_old.y = y
    ###END GL CALLBACKS




if __name__ == "__main__":
    p2 = window()



