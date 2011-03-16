#basic pyglet setup learned from here:
#http://www.learningpython.com/2007/11/10/creating-a-game-with-pyglet-and-python/

from pyglet import window
from pyglet import clock
from pyglet import font

from OpenGL.GL import *
from OpenGL.GLU import *

#helper modules
import glutil
from vector import Vec

#OpenCL code
import part2
import initialize

#number of particles
num = 200000
#time step for integration
dt = .01

class Part2Main(window.Window):
    def __init__(self, cle, *args, **kwargs):
        window.Window.__init__(self, *args, **kwargs)

        #glutil.init(self.width, self.height)
        self.cle = cle
        self.num = num

        #mouse handling for transforming scene
        self.rotate = Vec([0., 0., 0.])
        self.translate = Vec([0., 0., 0.])
        self.initrans = Vec([0., 0., -2.])


    def on_mouse_drag(self, x, y, dx, dy, buttons, modifiers):
        if buttons & window.mouse.LEFT:
            self.rotate.x += dy * .2
            self.rotate.y += dx * .2
        elif buttons & window.mouse.RIGHT:
            self.translate.z -= dy * .01 

    def set3d(self):
        glViewport(0, 0, self.width, self.height)
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        gluPerspective(60., self.width / float(self.height), .1, 1000.)
        glMatrixMode(GL_MODELVIEW)
    
    def unset3d(self):
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()


    def update(self):
        #execute our OpenCL kernel
        self.cle.execute()


    def draw(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        
        self.set3d()
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()

        #handle mouse transformations
        glTranslatef(self.initrans.x, self.initrans.y, self.initrans.z)
        glRotatef(-self.rotate.x, 1, 0, 0)
        glRotatef(self.rotate.y, 0, 1, 0) #we switched around the axis so make this rotate_z
        glTranslatef(self.translate.x, self.translate.y, self.translate.z)

        self.cle.render()
        
        #draw the x, y and z axis as lines
        glutil.draw_axes()

        glPopMatrix()
        self.unset3d()


    def main_loop(self):
        ft = font.load('Arial', 28)
        fps_text = font.Text(ft, y=10)

        while not self.has_exit:
            self.dispatch_events()

            self.update()
            self.draw()

            clock.tick()
            fps_text.text = "fps: %d" % clock.get_fps()
            fps_text.draw()

            self.flip()



if __name__ == "__main__":

    #set up initial conditions
    (pos_vbo, col_vbo, vel) = initialize.fountain(num)
    #create our OpenCL instance
    example = part2.Part2(num, dt, "part2.cl")
    example.loadData(pos_vbo, col_vbo, vel)


    #setup opengl context with double buffering in pyglet
    from pyglet import gl
    config = gl.Config()
    config.double_buffer=True
    config.depth_size = 16

    #create a window object and run our program!
    p2 = Part2Main(example, resizable=True, config=config)
    #glutil.lights()
    p2.main_loop()



