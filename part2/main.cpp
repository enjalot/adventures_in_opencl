/*
 * Adventures in OpenCL tutorial series
 * Part 2
 *
 * author: Ian Johnson
 * htt://enja.org
 * code based on advisor Gordon Erlebacher's work
 * NVIDIA's examples
 * as well as various blogs and resources on the internet
 */
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <math.h>


//OpenGL stuff
#include <GL/glew.h>
#if defined __APPLE__ || defined(MACOSX)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

//Our OpenCL Particle Systemclass
#include "cll.h"

#define NUM_PARTICLES 20000
CL* example;

//GL related variables
int window_width = 800;
int window_height = 600;
int glutWindowHandle = 0;
float translate_z = -1.f;
// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;
//main app helper functions
void init_gl(int argc, char** argv);
void appRender();
void appDestroy();
void timerCB(int ms);
void appKeyboard(unsigned char key, int x, int y);
void appMouse(int button, int state, int x, int y);
void appMotion(int x, int y);

//----------------------------------------------------------------------
//quick random function to distribute our initial points
float rand_float(float mn, float mx)
{
    float r = random() / (float) RAND_MAX;
    return mn + (mx-mn)*r;
}


//----------------------------------------------------------------------
int main(int argc, char** argv)
{
    printf("Hello, OpenCL\n");
    //Setup our GLUT window and OpenGL related things
    //glut callback functions are setup here too
    init_gl(argc, argv);

    //initialize our CL object, this sets up the context
    example = new CL();
    
    //load and build our CL program from the file
    #include "part2.cl" //std::string kernel_source is defined in this file
    example->loadProgram(kernel_source);

    //initialize our particle system with positions, velocities and color
    int num = NUM_PARTICLES;
    std::vector<Vec4> pos(num);
    std::vector<Vec4> vel(num);
    std::vector<Vec4> color(num);

    //fill our vectors with initial data
    for(int i = 0; i < num; i++)
    {
        //distribute the particles in a random circle around z axis
        float rad = rand_float(.2, .5);
        float x = rad*sin(2*3.14 * i/num);
        float z = 0.0f;// -.1 + .2f * i/num;
        float y = rad*cos(2*3.14 * i/num);
        pos[i] = Vec4(x, y, z, 1.0f);
        
        //give some initial velocity 
        //float xr = rand_float(-.1, .1);
        //float yr = rand_float(1.f, 3.f);
        //the life is the lifetime of the particle: 1 = alive 0 = dead
        //as you will see in part2.cl we reset the particle when it dies
        float life_r = rand_float(0.f, 1.f);
        vel[i] = Vec4(0.0, 0.0, 3.0f, life_r);

        //just make them red and full alpha
        color[i] = Vec4(1.0f, 0.0f,0.0f, 1.0f);
    }

    //our load data function sends our initial values to the GPU
    example->loadData(pos, vel, color);
    //initialize the kernel
    example->popCorn();
    
    //this starts the GLUT program, from here on out everything we want
    //to do needs to be done in glut callback functions
    glutMainLoop();

}


//----------------------------------------------------------------------
void appRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //this updates the particle system by calling the kernel
    example->runKernel();

    //render the particles from VBOs
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(5.);
    
    //printf("color buffer\n");
    glBindBuffer(GL_ARRAY_BUFFER, example->c_vbo);
    glColorPointer(4, GL_FLOAT, 0, 0);

    //printf("vertex buffer\n");
    glBindBuffer(GL_ARRAY_BUFFER, example->p_vbo);
    glVertexPointer(4, GL_FLOAT, 0, 0);

    //printf("enable client state\n");
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    //Need to disable these for blender
    glDisableClientState(GL_NORMAL_ARRAY);

    //printf("draw arrays\n");
    glDrawArrays(GL_POINTS, 0, example->num);

    //printf("disable stuff\n");
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glutSwapBuffers();
}


//----------------------------------------------------------------------
void init_gl(int argc, char** argv)
{

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition (glutGet(GLUT_SCREEN_WIDTH)/2 - window_width/2, 
                            glutGet(GLUT_SCREEN_HEIGHT)/2 - window_height/2);

    
    std::stringstream ss;
    ss << "Adventures in OpenCL: Part 2, " << NUM_PARTICLES << " particles" << std::ends;
    glutWindowHandle = glutCreateWindow(ss.str().c_str());

    glutDisplayFunc(appRender); //main rendering function
    glutTimerFunc(30, timerCB, 30); //determin a minimum time between frames
    glutKeyboardFunc(appKeyboard);
    glutMouseFunc(appMouse);
    glutMotionFunc(appMotion);

    glewInit();

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, window_width, window_height);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 1000.0);

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, translate_z);

}


//----------------------------------------------------------------------
void appDestroy()
{
    //this makes sure we properly cleanup our OpenCL context
    delete example;
    if(glutWindowHandle)glutDestroyWindow(glutWindowHandle);
    printf("about to exit!\n");

    exit(0);
}


//----------------------------------------------------------------------
void timerCB(int ms)
{
    //this makes sure the appRender function is called every ms miliseconds
    glutTimerFunc(ms, timerCB, ms);
    glutPostRedisplay();
}


//----------------------------------------------------------------------
void appKeyboard(unsigned char key, int x, int y)
{
    //this way we can exit the program cleanly
    switch(key)
    {
        case '\033': // escape quits
        case '\015': // Enter quits    
        case 'Q':    // Q quits
        case 'q':    // q (or escape) quits
            // Cleanup up and quit
            appDestroy();
            break;
    }
}


//----------------------------------------------------------------------
void appMouse(int button, int state, int x, int y)
{
    //handle mouse interaction for rotating/zooming the view
    if (state == GLUT_DOWN) {
        mouse_buttons |= 1<<button;
    } else if (state == GLUT_UP) {
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;
}


//----------------------------------------------------------------------
void appMotion(int x, int y)
{
    //hanlde the mouse motion for zooming and rotating the view
    float dx, dy;
    dx = x - mouse_old_x;
    dy = y - mouse_old_y;

    if (mouse_buttons & 1) {
        rotate_x += dy * 0.2;
        rotate_y += dx * 0.2;
    } else if (mouse_buttons & 4) {
        translate_z += dy * 0.1;
    }

    mouse_old_x = x;
    mouse_old_y = y;

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, translate_z);
    glRotatef(rotate_x, 1.0, 0.0, 0.0);
    glRotatef(rotate_y, 0.0, 1.0, 0.0);
}



