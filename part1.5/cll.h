#ifndef ADVCL_CLL_H_INCLUDED
#define ADVCL_CLL_H_INCLUDED

#define __CL_ENABLE_EXCEPTIONS

#include <vector>
#include "CL/cl.hpp"

class CL {
    public:

        //These are arrays we will use in this tutorial
        ///cl_mem cl_a;
        ///cl_mem cl_b;
        ///cl_mem cl_c;
        cl::Buffer cl_a;
        cl::Buffer cl_b;
        cl::Buffer cl_c;
        int num;    //the size of our arrays


        //size_t workGroupSize[1]; //N dimensional array of workgroup size we must pass to the kernel

        //default constructor initializes OpenCL context and automatically chooses platform and device
        CL();
        //default destructor releases OpenCL objects and frees device memory
        ~CL();

        //load an OpenCL program from a file
        //pass in the kernel source code as a string. handy way to get this from STRINGIFY macro in part1.cl
        void loadProgram(std::string kernel_source);

        //setup the data for the kernel 
        //these are implemented in part1.cpp (in the future we will make these more general)
        void popCorn();
        //execute the kernel
        void runKernel();

    private:

        //handles for creating an opencl context 
        //cl_platform_id platform;
        
        //device variables
        //cl_device_id* devices;
        //cl_uint numDevices;
        unsigned int deviceUsed;
        std::vector<cl::Device> devices;
        
        //cl_context context;
        cl::Context context;

        //cl_command_queue command_queue;
        cl::CommandQueue queue;
        //cl_program program;
        cl::Program program;
        //cl_kernel kernel;
        cl::Kernel kernel;
        

        //debugging variables
        cl_int err;
        ///cl_event event;
        cl::Event event;
        
        //buildExecutable is called by loadProgram
        //build runtime executable from a program
        //void buildExecutable();
};

#endif
