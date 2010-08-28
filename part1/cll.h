#ifndef ADVCL_CLL_H_INCLUDED
#define ADVCL_CLL_H_INCLUDED

#if defined __APPLE__ || defined(MACOSX)
    #include <OpenCL/opencl.h>
#else
    #include <CL/opencl.h>
#endif

class CL {
    public:

        //These are arrays we will use in this tutorial
        cl_mem cl_a;
        cl_mem cl_b;
        cl_mem cl_c;
        int num;    //the size of our arrays


        size_t workGroupSize[1]; //N dimensional array of workgroup size we must pass to the kernel

        //default constructor initializes OpenCL context and automatically chooses platform and device
        CL();
        //default destructor releases OpenCL objects and frees device memory
        ~CL();

        //load an OpenCL program from a file
        //the path is relative to the CL_SOURCE_DIR set in CMakeLists.txt
        void loadProgram(const char* relative_path);

        //setup the data for the kernel 
        //these are implemented in part1.cpp (in the future we will make these more general)
        void popCorn();
        //execute the kernel
        void runKernel();

    private:

        //handles for creating an opencl context 
        cl_platform_id platform;
        
        //device variables
        cl_device_id* devices;
        cl_uint numDevices;
        unsigned int deviceUsed;
        
        cl_context context;

        cl_command_queue command_queue;
        cl_program program;
        cl_kernel kernel;
        

        //debugging variables
        cl_int err;
        cl_event event;
        
        //buildExecutable is called by loadProgram
        //build runtime executable from a program
        void buildExecutable();
};

#endif
