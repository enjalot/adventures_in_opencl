from OpenGL.GL import *

import numpy as np

import timing
timings = timing.Timing()

#@timings
def ca_np(ntracers):

    pos = np.ones((ntracers, 4), dtype=np.float32)
    col = np.ones((ntracers, 4), dtype=np.float32)
    time = np.zeros((ntracers, 4), dtype=np.float32)
    props = np.zeros((ntracers, 4), dtype=np.int32)

    pos[0] = np.array([.5,.5,0.,1.], dtype=np.float32)
    col[0] = np.array([0.,1.,0.,1.], dtype=np.float32)
        
    return pos, col, time, props
    

def ca(ntracers):
    """Initialize position, color and velocity arrays we also make Vertex
    Buffer Objects for the position and color arrays"""
    pos, col, time, props = ca_np(ntracers)
    
    #print timings

    #create the Vertex Buffer Objects
    from OpenGL.arrays import vbo 
    pos_vbo = vbo.VBO(data=pos, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
    pos_vbo.bind()
    col_vbo = vbo.VBO(data=col, usage=GL_DYNAMIC_DRAW, target=GL_ARRAY_BUFFER)
    col_vbo.bind()

    return (pos_vbo, col_vbo, time, props)


