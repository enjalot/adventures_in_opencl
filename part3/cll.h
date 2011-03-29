#ifndef ADVCL_CLL_H_INCLUDED
#define ADVCL_CLL_H_INCLUDED

#define __CL_ENABLE_EXCEPTIONS

#include <vector>
#include "CL/cl.hpp"


//Define the struct we will be sending to OpenCL as parameters
//the attribute alligned makes it always have a size that is a 
//multiple of 16 bytes
typedef struct Params
{
    float A;
    float B;
    float x;  //padding
    float xx; //padding
    int C;
} Params __attribute__((aligned(16)));



class CL {
    public:

        //cpu side arrays
        std::vector<float> a, b;
        //These are arrays we will use in this tutorial
        cl::Buffer cl_a;
        cl::Buffer cl_b;
        cl::Buffer cl_c;
        int num;    //the size of our arrays
 
        Params params;
        cl::Buffer cl_params;

        //default constructor initializes OpenCL context and automatically chooses platform and device
        CL();
        //default destructor releases OpenCL objects and frees device memory
        ~CL();

        //load an OpenCL program from a file
        //pass in the kernel source code as a string. handy way to get this from STRINGIFY macro in part1.cl
        cl::Program loadProgram(std::string path);

        //setup the data for the kernel 
        //these are implemented in part1.cpp (in the future we will make these more general)
        void popCorn();
        //execute the kernel
        void runKernel();

    private:

        //device variables
        unsigned int deviceUsed;
        std::vector<cl::Device> devices;
        
        cl::Context context;

        cl::CommandQueue queue;
        cl::Program program;
        cl::Kernel kernel;
       
        //debugging variables
        cl_int err;
        ///cl_event event;
        cl::Event event;
};

#endif
