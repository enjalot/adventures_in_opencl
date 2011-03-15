from OpenGL.GL import *
from OpenGL.GLU import *

from vector import Vec


def init(width, height):

    #glEnable(GL_DEPTH_TEST)
    glEnable(GL_NORMALIZE)
    glShadeModel(GL_SMOOTH)


    glViewport(0, 0, width, height)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(60.0, width/float(height), .1, 8192)
    #glEnable(GL_DEPTH_TEST)
    glMatrixMode(GL_MODELVIEW)



def lights():
    glEnable(GL_LIGHTING)
    glEnable(GL_COLOR_MATERIAL)

    light_position = [10., 10., 200., 0.]
    light_ambient = [.2, .2, .2, 1.]
    light_diffuse = [.6, .6, .6, 1.]
    light_specular = [2., 2., 2., 0.]
    glLightfv(GL_LIGHT0, GL_POSITION, light_position)
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient)
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse)
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular)
    glEnable(GL_LIGHT0)

    mat_ambient = [.2, .2, 1.0, 1.0]
    mat_diffuse = [.2, .8, 1.0, 1.0]
    mat_specular = [1.0, 1.0, 1.0, 1.0]
    high_shininess = 3.

    mat_ambient_back = [.5, .2, .2, 1.0]
    mat_diffuse_back = [1.0, .2, .2, 1.0]

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    glMaterialfv(GL_BACK, GL_AMBIENT,   mat_ambient_back);
    glMaterialfv(GL_BACK, GL_DIFFUSE,   mat_diffuse_back);
    glMaterialfv(GL_BACK, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_BACK, GL_SHININESS, high_shininess);



def draw_line(v1, v2):
    glBegin(GL_LINES)
    glVertex3f(v1.x, v1.y, v1.z)
    glVertex3f(v2.x, v2.y, v2.z)
    glEnd()


def draw_axes():
    #X Axis
    glColor3f(1,0,0)    #red
    v1 = Vec([0,0,0])
    v2 = Vec([1,0,0])
    draw_line(v1, v2)

    #Y Axis
    glColor3f(0,1,0)    #green
    v1 = Vec([0,0,0])
    v2 = Vec([0,1,0])
    draw_line(v1, v2)

    #Z Axis
    glColor3f(0,0,1)    #blue
    v1 = Vec([0,0,0])
    v2 = Vec([0,0,1])
    draw_line(v1, v2)



