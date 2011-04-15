from OpenGL.GL import *

import numpy
import scipy.signal

import timing
timings = timing.Timing()

#@timings
def wave_np(wtype, dt, dx, ntracers):

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

        z = 0.# - it * dx

        pos[t:tn,0] = xs 
        if wtype == "sin":
            pos[t:tn,1] = numpy.sin(xs * 4.001 * numpy.pi) 
        elif wtype == "square":
            pos[t:tn,1] = .75*scipy.signal.square(xs * 4.001 * numpy.pi) 
        elif wtype == "sawtooth":
            pos[t:tn,1] = .75*scipy.signal.sawtooth(xs * 8.001 * numpy.pi) 
        elif wtype == "sweep_poly":
            poly = numpy.poly1d([1,2,0,2,3])
            pos[t:tn,1] = .75*scipy.signal.sweep_poly(xs, poly)
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
    


