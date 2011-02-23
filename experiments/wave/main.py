#Port from Adventures in OpenCL Part2 to PyOpenCL
# http://enja.org/2010/08/27/adventures-in-opencl-part-2-particles-with-opengl/
#Author: Ian Johnson
#referenced: 
# http://documen.tician.de/pyopencl/
# http://www.geometrian.com/Tutorials.php

from OpenGL.GL import *
#import OpenGL.GL as gl
from OpenGL.GLU import *

import pygame
from pygame.locals import *

#utility functions for drawing OpenGL stuff
import glutil
#wrapper for numpy array that gives us float4 like behavior
from vector import Vec

import os, sys

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

import wave
glutil.init(screen)

subintervals = 20
#dt = .0002
#dx = .001
dt = .002*.02
dx = .024*.02
wv = wave.Wave(dt, dx, subintervals)

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

    wv.execute()
    wv.render()
    
    glutil.draw_axes()

    pygame.display.flip()


def main():

    clock = pygame.time.Clock()

    while True:
        clock.tick(60)
        get_input()
        draw()


if __name__ == '__main__': main()
