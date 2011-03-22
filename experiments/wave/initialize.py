from OpenGL.GL import *

import numpy

import timing
timings = timing.Timing()

#@timings
def wave_np(dt, dx, ntracers):

    xs = numpy.arange(0., 1. + dx, dx)
    num = len(xs)

    #ntracers + 1 because original array is not considered a tracer
    pos = numpy.ndarray((num*(ntracers+1), 4), dtype=numpy.float32)
    col = numpy.ndarray((num*(ntracers+1), 4), dtype=numpy.float32)
    vel = numpy.ndarray((num*(ntracers+1), 4), dtype=numpy.float32)
        
    for it in xrange(0, ntracers+1):
        #print "z", z
        t = it*num
        tn = t + num
        #print it, t, tn

        z = 0. - it * dx

        pos[t:tn,0] = xs 
        pos[t:tn,1] = numpy.sin(xs * 4.001 * numpy.pi) 
        pos[t:tn,2] = z
        pos[t:tn,3] = ntracers*dt - it*dt   #for calculating the life of a tracer particle


        #ymaxm = ymaxm < 0. ? -ymaxm : ymaxm;
        #color[i].x = 1 - ymaxm;
        #color[i].y = ymaxm;

        
        col[t:tn,0] = -1. * pos[t:tn,1] + 1.
        col[t:tn,1] = 1. * numpy.abs(pos[t:tn,1]) + 1.
        col[t:tn,2] = 0.
        col[t:tn,3] = 1.


    print "pos.shape", pos.shape
    return pos, col, num
    

def wave(dt, dx, ntracers):
    """Initialize position, color and velocity arrays we also make Vertex
    Buffer Objects for the position and color arrays"""

    choice = 3
    if choice == 1:        #linear
        #c = 10.
        c = 1.
        param = c
        ymin = -150.
        ymax = 150.

    elif choice == 2:      #quadratic 
        #unstable for quadratic
        #beta = .016568
        beta = .0016568
        param = beta
        ymin = -12.
        ymax = 12.

    elif choice == 3:      #cubic
        #gamma = .509
        gamma = .0509
        #gamma = .00509
        param = gamma
        ymin = -1.
        ymax = 1.

    pos, col, num = wave_np(dt, dx, ntracers)
    
    #print timings

    #create the Vertex Buffer Objects
    from OpenGL.arrays import vbo 
    pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
    pos_vbo.bind()
    col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
    col_vbo.bind()

    params = (num, choice, param, ymin, ymax)
    return (pos_vbo, col_vbo, params)


