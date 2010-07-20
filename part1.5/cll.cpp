#include <stdio.h>
#include <string>
#include <iostream>

#include "cll.h"
#include "util.h"

CL::CL()
{
    printf("Initialize OpenCL object and context\n");
    //setup devices and context
    
    //this function is defined in util.cpp
    //it comes from the NVIDIA SDK example code
    ///err = oclGetPlatformID(&platform);
    //oclErrorString is also defined in util.cpp and comes from the NVIDIA SDK
    ///printf("oclGetPlatformID: %s\n", oclErrorString(err));
    std::vector<cl::Platform> platforms;
    err = cl::Platform::get(&platforms);
    printf("cl::Platform::get(): %s\n", oclErrorString(err));
    if (platforms.size() == 0) {
        printf("Platform size 0\n");
    }
 

    // Get the number of GPU devices available to the platform
    // we should probably expose the device type to the user
    // the other common option is CL_DEVICE_TYPE_CPU
    ///err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    ///printf("clGetDeviceIDs (get number of devices): %s\n", oclErrorString(err));


    // Create the device list
    ///devices = new cl_device_id [numDevices];
    ///err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    ///printf("clGetDeviceIDs (create device list): %s\n", oclErrorString(err));
 

    //for right now we just use the first available device
    //later you may have criteria (such as support for different extensions)
    //that you want to use to select the device
    deviceUsed = 0;
    //create the context
    ///context = clCreateContext(0, 1, &devices[deviceUsed], NULL, NULL, &err);
    //context properties will be important later, for now we go with defualts
    cl_context_properties properties[] = 
        { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};

    context = cl::Context(CL_DEVICE_TYPE_GPU, properties);
    devices = context.getInfo<CL_CONTEXT_DEVICES>();
    
    //create the command queue we will use to execute OpenCL commands
    ///command_queue = clCreateCommandQueue(context, devices[deviceUsed], 0, &err);
    try{
        queue = cl::CommandQueue(context, devices[deviceUsed], 0, &err);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%d)\n", er.what(), er.err());
    }

}

CL::~CL()
{
    /*
    printf("Releasing OpenCL memory\n");
    if(kernel)clReleaseKernel(kernel); 
    if(program)clReleaseProgram(program);
    if(command_queue)clReleaseCommandQueue(command_queue);
    //need to release any other OpenCL memory objects here
    if(cl_a)clReleaseMemObject(cl_a);
    if(cl_b)clReleaseMemObject(cl_b);
    if(cl_c)clReleaseMemObject(cl_c);

    if(context)clReleaseContext(context);
    
    if(devices)delete(devices);
    printf("OpenCL memory released\n");
    
    */
}


void CL::loadProgram(const char* kernel_source)
{
 // Program Setup
    int pl;
    //size_t program_length;
    printf("load the program\n");
    
    //CL_SOURCE_DIR is set in the CMakeLists.txt
    /*
    std::string path(CL_SOURCE_DIR);
    path += "/" + std::string(relative_path);
    printf("path: %s\n", path.c_str());
    */
    //file_contents is defined in util.cpp
    //it loads the contents of the file at the given path
    //char* cSourceCL = file_contents(path.c_str(), &pl);
    //#include "part1.cl"
    cl::Program::Sources source(1,
        std::make_pair(kernel_source,pl));
    
    program = cl::Program(context, source);

    err = program.build(devices);
    printf("program.build: %s\n", oclErrorString(err));
    if(err != CL_SUCCESS){
		std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
		std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
		std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
	} 
    //std::string str = program.getBuildInfo(devices[deviceUsed]);
    //printf("Build Log:\n %s \n", str.c_str());

    //printf("file: %s\n", cSourceCL);
    /*
    program_length = (size_t)pl;

    // create the program
    program = clCreateProgramWithSource(context, 1,
                      (const char **) &cSourceCL, &program_length, &err);
    printf("clCreateProgramWithSource: %s\n", oclErrorString(err));

    buildExecutable();
    */
}

/*
//----------------------------------------------------------------------
void CL::buildExecutable()
{
    // Build the program executable
    
    printf("building the program\n");
    // build the program
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    printf("clBuildProgram: %s\n", oclErrorString(err));
	if(err != CL_SUCCESS){
		cl_build_status build_status;
		err = clGetProgramBuildInfo(program, devices[deviceUsed], CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);

		char *build_log;
		size_t ret_val_size;
		err = clGetProgramBuildInfo(program, devices[deviceUsed], CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);

		build_log = new char[ret_val_size+1];
		err = clGetProgramBuildInfo(program, devices[deviceUsed], CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
		build_log[ret_val_size] = '\0';
		printf("BUILD LOG: \n %s", build_log);
	}

    //printf("program built\n");
}
*/

