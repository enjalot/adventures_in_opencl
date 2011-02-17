#Port from Adventures in OpenCL Part2 to PyOpenCL
# http://enja.org/2010/08/27/adventures-in-opencl-part-2-particles-with-opengl/
#Author: Ian Johnson
#referenced: 
# http://documen.tician.de/pyopencl/
# http://www.geometrian.com/Tutorials.php

from OpenGL.GL import *
from OpenGL.GLU import *

import pygame
from pygame.locals import *

#utility functions for drawing OpenGL stuff
import glutil as gl
#wrapper for numpy array that gives us float4 like behavior
from vector import Vec

import os, sys
from math import sqrt, sin, cos

pygame.init()
pygame.display.set_caption("PyOpenCL with PyOpenGL interop")
screen = (800, 600)
surface = pygame.display.set_mode(screen, OPENGL|DOUBLEBUF)

#should just have an interaction class for controlling the window
#global mouse_old, rotate, translate, mouse_down
mouse_down = False
mouse_old = Vec([0.,0.])
rotate = Vec([0., 0., 0.])
translate = Vec([0., 0., 0.])
initrans = Vec([0, 0, -2])

gl.init(screen)

num = 20000
#setup initial values of arrays
import numpy
pos = numpy.ndarray((num, 4), dtype=numpy.float32)
col = numpy.ndarray((num, 4), dtype=numpy.float32)
vel = numpy.ndarray((num, 4), dtype=numpy.float32)

import random
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

#for some reason trying to do this inside CL.loadData gives me errors on mac
from OpenGL.arrays import vbo
pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
pos_vbo.bind()
col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
col_vbo.bind()
 

import part2
example = part2.CL()
example.loadProgram("part2.cl")
example.loadData(pos_vbo, col_vbo, vel)
#print example.pos_vbo.data


def get_input():
    global mouse_down, mouse_old, translate, rotate
    key = pygame.key.get_pressed()
    #print key
    trans = 2.0

    for event in pygame.event.get():
        if event.type == QUIT or key[K_ESCAPE] or key[K_q]:
            print "quit!"
            pygame.quit(); sys.exit()

        elif event.type == MOUSEBUTTONDOWN:
            mouse_down = True
            mouse_old = Vec([event.pos[0]*1., event.pos[1]*1.])

        elif event.type == MOUSEMOTION:
            if(mouse_down):
                m = Vec([event.pos[0]*1., event.pos[1]*1.])
                dx = m.x - mouse_old.x
                dy = m.y - mouse_old.y
                button1, button2, button3 = pygame.mouse.get_pressed()
                if button1:
                    rotate.x += dy * .2
                    rotate.y += dx * .2
                elif button3:
                    translate .z -= dy * .01 

                mouse_old = m
                #print "rotate", rotate, "translate", translate

        elif event.type == MOUSEBUTTONUP:
            mouse_down = False
        elif key[K_w]:
            translate.z += .1*trans   #y is z and z is y
        elif key[K_s]:
            translate.z -= .1*trans
        elif key[K_a]:
            translate.x += .1*trans
        elif key[K_d]:
            translate.x -= .1*trans

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()
    glTranslatef(initrans.x, initrans.y, initrans.z)
    glRotatef(rotate.x, 1, 0, 0)
    glRotatef(rotate.y, 0, 1, 0) #we switched around the axis so make this rotate_z
    glTranslatef(translate.x, translate.y, translate.z)


def draw():
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)

    example.execute()

    #glColor3f(1,0,0)
    glEnable(GL_POINT_SMOOTH)
    glPointSize(5)

    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    example.col_vbo.bind()
    glColorPointer(4, GL_FLOAT, 0, example.col_vbo)

    example.pos_vbo.bind()
    glVertexPointer(4, GL_FLOAT, 0, example.pos_vbo)

    glEnableClientState(GL_VERTEX_ARRAY)
    glEnableClientState(GL_COLOR_ARRAY)
    glDrawArrays(GL_POINTS, 0, num)

    glDisableClientState(GL_COLOR_ARRAY)
    glDisableClientState(GL_VERTEX_ARRAY)

    glDisable(GL_BLEND)
    
    gl.draw_axes()

    pygame.display.flip()


def main():

    clock = pygame.time.Clock()

    while True:
        clock.tick(60)
        get_input()
        draw()


if __name__ == '__main__': main()
