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
import wave
#functions for initial values of particles
import initialize

#number of particles
#num = 20000
subintervals = 30
ntracers = 300
#dt = .0001
#dx = .002
dt = .002*.01
dx = .024*.1

class window(object):
    def __init__(self, *args, **kwargs):
        #mouse handling for transforming scene
        self.mouse_down = False
        self.mouse_old = Vec([0., 0.])
        self.rotate = Vec([0., 0., 0.])
        self.translate = Vec([0., 0., 0.])
        #self.initrans = Vec([0., 0., -2.])
        self.init_persp_trans = Vec([-.5, 0., -1.5])
        self.init_ortho_trans = Vec([0., 0., 0.])
        self.init_persp_rotate = Vec([0., 0., 0.])
        self.init_ortho_rotate = Vec([90., -90., 0.])
 

        self.ortho = False
        self.choice = 2
        #self.stable = True
        self.stable = False
        #self.type = "square"
        self.wtype = "sin"
        #self.wtype = "sawtooth"
        #self.wtype = "sweep_poly"
        self.dt = dt

        self.width = 640
        self.height = 480

        glutInit(sys.argv)
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
        glutInitWindowSize(self.width, self.height)
        glutInitWindowPosition(0, 0)
        self.win = glutCreateWindow("Part 2: Python")

        #gets called by GLUT every frame
        glutDisplayFunc(self.draw)

        #handle user input
        glutKeyboardFunc(self.on_key)
        glutMouseFunc(self.on_click)
        glutMotionFunc(self.on_mouse_motion)
        
        #this will call draw every 30 ms
        glutTimerFunc(30, self.timer, 30)

        glViewport(0, 0, self.width, self.height)
        #setup OpenGL scene
        self.glprojection()

        #set up initial conditions
        self.init_wave(self.dt, dx, ntracers, True)
        #create our OpenCL instance
        #self.cle = part2.Part2(num, dt, "part2.cl")
        self.cle = wave.Wave(self.dt, dx, ntracers, self.params)
        self.cle.loadData(self.pos_vbo, self.col_vbo)

        self.cle.execute(subintervals) 
        glutMainLoop()
 
    def set_params(self):
        if self.choice == 1:        #linear
            if not self.stable:
                c = 1 #unstable
            else:
                c = 10. #stable
            param = c
            ymin = -150.
            ymax = 150.

        elif self.choice == 2:      #quadratic 
            if not self.stable:
                #unstable for quadratic
                beta = .016568
            else:
                beta = .0016568     #stable
            param = beta
            ymin = -12.
            ymax = 12.

        elif self.choice == 3:      #cubic
            if not self.stable:
                #gamma = .509
                gamma = .0509  #unstable
            else:
                gamma = .00509  #stable
            param = gamma
            ymin = -1.
            ymax = 1.
        
        self.params = (self.num, self.choice, param, ymin, ymax, self.dt)

        print "Parameters: ======================"
        if self.choice == 1:
            print "Linear Wave Equation"
        if self.choice == 2:
            print "Quadratic Wave Equation"
        if self.choice == 3:
            print "Cubic Wave Equation"
        if self.stable:
            print "Stability: Stable"
        else:
            print "Stability: Unstable"
        print "%s wave" % self.wtype
        print "Timestep: ", self.dt
        print "=================================="



    def init_wave(self, dt, dx, numtracers, init):
        """Initialize position, color and velocity arrays we also make Vertex
        Buffer Objects for the position and color arrays"""

        pos, col, self.num = initialize.wave_np(self.wtype, dt, dx, numtracers)
        self.set_params()
        
        #print timings

        if init:
            #create the Vertex Buffer Objects
            from OpenGL.arrays import vbo 
            self.pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
            self.pos_vbo.bind()
            self.col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
            self.col_vbo.bind()
        else:
            self.pos_vbo.bind()
            self.pos_vbo.set_array(pos)
            self.pos_vbo.copy_data()
            self.col_vbo.bind()
            self.col_vbo.set_array(col)
            self.col_vbo.copy_data()
            self.cle.set_params(self.params)
            self.cle.reloadData()

       

        #return (pos_vbo, col_vbo, params)

       

    def draw(self):
        """Render the particles"""        
        #TODO: 
        # * set up Ortho2D viewing and mouse controls
        # * find better color mapping for height

        
        #update or particle positions by calling the OpenCL kernel
        self.cle.execute(subintervals) 
        glFlush()

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        #handle mouse transformations
        #glTranslatef(self.initrans.x, self.initrans.y, self.initrans.z)
        glRotatef(self.rotate.x, 1, 0, 0)
        glRotatef(self.rotate.y, 0, 1, 0) 
        glTranslatef(self.translate.x, self.translate.y, self.translate.z)
        
        #render the particles
        self.cle.render()

        #draw the x, y and z axis as lines
        glutil.draw_axes()

        glutSwapBuffers()


    def glprojection(self):
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()

        if self.ortho:
            glOrtho(0.0, 1.0, 0.0, -1.0, -1.5, 1.5)
            self.translate = self.init_ortho_trans.copy()
            self.rotate = self.init_ortho_rotate.copy()
        else:
            gluPerspective(60., self.width / float(self.height), .1, 1000.)
            self.translate= self.init_persp_trans.copy()
            self.rotate = self.init_persp_rotate.copy()

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
        elif args[0] == 'o':
            self.ortho = not self.ortho
            if self.ortho:
                self.translate = self.init_ortho_trans.copy()
                self.rotate = self.init_ortho_rotate.copy()
            else:
                self.translate = self.init_persp_trans.copy()
                self.rotate = self.init_persp_rotate.copy()
            self.glprojection()
        elif args[0] == '1':
            self.choice = 1
            self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == '2':
            self.choice = 2
            self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == '3':
            self.choice = 3
            self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == 's':
            self.stable = not self.stable
            #print "Stable parameters: ", self.stable
            self.set_params()
            self.cle.set_params(self.params)
            #self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == 'v':
            self.wtype = "sin"
            self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == 'b':
            self.wtype = "sawtooth"
            self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == 'n':
            self.wtype = "square"
            self.init_wave(self.dt, dx, ntracers, False)
        elif args[0] == '-':
            self.dt *= .1
            self.set_params()
            self.cle.set_params(self.params)
        elif args[0] == '=':
            self.dt *= 10
            self.set_params()
            self.cle.set_params(self.params)
        """
        elif args[0] == 'm':
            self.wtype = "sweep_poly"
            self.init_wave(self.dt, dx, ntracers, False)
        """
        







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
            #self.rotate.y += dx * .2
        elif self.mouse_down and self.button == 2: #right button
            self.translate.z -= dy * .01 
        self.mouse_old.x = x
        self.mouse_old.y = y
    ###END GL CALLBACKS




if __name__ == "__main__":
    p2 = window()



