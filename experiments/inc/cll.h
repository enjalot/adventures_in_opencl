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
    float c1;
    float c2;
} Params __attribute__((aligned(16)));



class CL {
    public:

        //These are arrays we will use in this tutorial
        cl::Buffer cl_a;
        cl::Buffer cl_b;
        cl::Buffer cl_c_a;
        cl::Buffer cl_c_b;
        int num;    //the size of our arrays

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
        cl::Program a_program, b_program;
        cl::Kernel a_kernel, b_kernel;
        
        Params params;
        cl::Buffer cl_params;

        //debugging variables
        cl_int err;
        ///cl_event event;
        cl::Event event;
};

#endif
