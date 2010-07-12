#include <string>

#include "cll.h"
#include "util.h"

CL::CL()
{
    printf("Initialize OpenCL object and context\n");
    //setup devices and context
    
    //this function is defined in util.cpp
    //it comes from the NVIDIA SDK example code
    err = oclGetPlatformID(&platform);
    //oclErrorString is also defined in util.cpp and comes from the NVIDIA SDK
    printf("oclGetPlatformID: %s\n", oclErrorString(err));

    // Get the number of GPU devices available to the platform
    // we should probably expose the device type to the user
    // the other common option is CL_DEVICE_TYPE_CPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    printf("clGetDeviceIDs (get number of devices): %s\n", oclErrorString(err));


    // Create the device list
    devices = new cl_device_id [numDevices];
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    printf("clGetDeviceIDs (create device list): %s\n", oclErrorString(err));
 

    //for right now we just use the first available device
    //later you may have criteria (such as support for different extensions)
    //that you want to use to select the device
    deviceUsed = 0;
    //create the context
    context = clCreateContext(0, 1, &devices[deviceUsed], NULL, NULL, &err);
    
    //create the command queue we will use to execute OpenCL commands
    command_queue = clCreateCommandQueue(context, devices[deviceUsed], 0, &err);

}

CL::~CL()
{
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
}


void CL::loadProgram(const char* relative_path)
{
 // Program Setup
    int pl;
    size_t program_length;
    printf("load the program\n");
    
    //CL_SOURCE_DIR is set in the CMakeLists.txt
    std::string path(CL_SOURCE_DIR);
    path += "/" + std::string(relative_path);
    printf("path: %s\n", path.c_str());

    //file_contents is defined in util.cpp
    //it loads the contents of the file at the given path
    char* cSourceCL = file_contents(path.c_str(), &pl);
    //printf("file: %s\n", cSourceCL);
    program_length = (size_t)pl;

    // create the program
    program = clCreateProgramWithSource(context, 1,
                      (const char **) &cSourceCL, &program_length, &err);
    printf("clCreateProgramWithSource: %s\n", oclErrorString(err));

    buildExecutable();
   
}

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
    printf("program built\n");
}


/*
int EnjaParticles::update(float dt)
{
    cl_int ciErrNum = CL_SUCCESS;
    cl_event evt; //can't do opencl visual profiler without passing an event

 
#ifdef GL_INTEROP   
    // map OpenGL buffer object for writing from OpenCL
    //clFinish(cqCommandQueue);
    ts_cl[3]->start();
    glFinish();
    ts_cl[3]->stop();

	ts_cl[0]->start();
    //ciErrNum = clEnqueueAcquireGLObjects(cqCommandQueue, 1, &vbo_cl, 0,0,0);
    ciErrNum = clEnqueueAcquireGLObjects(cqCommandQueue, 2, cl_vbos, 0,NULL, &evt);
    clReleaseEvent(evt);
    //printf("gl interop, acquire: %s\n", oclErrorString(ciErrNum));
    clFinish(cqCommandQueue);
	ts_cl[0]->stop();
#endif

    //clFinish(cqCommandQueue);
	ts_cl[1]->start();
    ciErrNum = clSetKernelArg(ckKernel, 5, sizeof(float), &dt);
    //ciErrNum = clSetKernelArg(ckKernel, 2, sizeof(float), &dt);
    ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 1, NULL, szGlobalWorkSize, NULL, 0, NULL, &evt );
    clReleaseEvent(evt);
    //printf("enqueueue nd range kernel: %s\n", oclErrorString(ciErrNum));
    clFinish(cqCommandQueue); //wont get reliable timings unless we finish the queue for each action
    ts_cl[1]->stop();

#ifdef GL_INTEROP
    // unmap buffer object
    //ciErrNum = clEnqueueReleaseGLObjects(cqCommandQueue, 1, &vbo_cl, 0,0,0);
    
    //clFinish(cqCommandQueue);
    ts_cl[2]->start();
    ciErrNum = clEnqueueReleaseGLObjects(cqCommandQueue, 2, cl_vbos, 0, NULL, &evt);
    clReleaseEvent(evt);


    //printf("gl interop, acquire: %s\n", oclErrorString(ciErrNum));
    clFinish(cqCommandQueue);
    ts_cl[2]->stop();
#else

    // Explicit Copy 
    // this doesn't get called when we use GL_INTEROP
    glBindBufferARB(GL_ARRAY_BUFFER, v_vbo);    
    // map the buffer object into client's memory
    void* ptr = glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);
    ciErrNum = clEnqueueReadBuffer(cqCommandQueue, cl_vbos[0], CL_TRUE, 0, vbo_size, ptr, 0, NULL, &evt);
    clReleaseEvent(evt);
    glUnmapBufferARB(GL_ARRAY_BUFFER); 
    
    glBindBufferARB(GL_ARRAY_BUFFER, c_vbo);    
    // map the buffer object into client's memory
    ptr = glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);
    ciErrNum = clEnqueueReadBuffer(cqCommandQueue, cl_vbos[1], CL_TRUE, 0, vbo_size, ptr, 0, NULL, &evt);
    clReleaseEvent(evt);
    glUnmapBufferARB(GL_ARRAY_BUFFER); 
#endif


}


void EnjaParticles::popCorn()
{


    cl_event evt; //can't do opencl visual profiler without passing an event
    //This is a purely internal helper function, all this code could easily be at the bottom of init_cl
    //init_cl shouldn't change much, and this may
    #ifdef GL_INTEROP
        printf("gl interop!\n");
        // create OpenCL buffer from GL VBO
        cl_vbos[0] = clCreateFromGLBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, v_vbo, &ciErrNum);
        cl_vbos[1] = clCreateFromGLBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, c_vbo, &ciErrNum);
        //printf("SUCCES?: %s\n", oclErrorString(ciErrNum));
    #else
        printf("no gl interop!\n");
        // create standard OpenCL mem buffer
        cl_vbos[0] = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, vbo_size, NULL, &ciErrNum);
        cl_vbos[1] = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, vbo_size, NULL, &ciErrNum);
        //Since we don't get the data from OpenGL we have to manually push the CPU side data to the GPU
        ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cl_vbos[0], CL_TRUE, 0, vbo_size, generators, 0, NULL, &evt);
        clReleaseEvent(evt);
        ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cl_vbos[1], CL_TRUE, 0, vbo_size, colors, 0, NULL, &evt);
        clReleaseEvent(evt);
        //make sure we are finished copying over before going on
    #endif
    
    //support arrays for the particle system
    cl_generators = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, vbo_size, NULL, &ciErrNum);
    cl_velocities= clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, vbo_size, NULL, &ciErrNum);
    cl_life = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, sizeof(float) * num, NULL, &ciErrNum);
    
    ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cl_generators, CL_TRUE, 0, vbo_size, generators, 0, NULL, &evt);
    clReleaseEvent(evt);
    ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cl_velocities, CL_TRUE, 0, vbo_size, velocities, 0, NULL, &evt);
    clReleaseEvent(evt);
    ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cl_life, CL_TRUE, 0, sizeof(float) * num, life, 0, NULL, &evt);
    clReleaseEvent(evt);
    clFinish(cqCommandQueue);
    
    printf("lorentz kernel\n");

    //printf("about to set kernel args\n");
    ciErrNum  = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void *) &cl_vbos[0]);  //vertices is first arguement to kernel
    ciErrNum  = clSetKernelArg(ckKernel, 1, sizeof(cl_mem), (void *) &cl_vbos[1]);  //colors is second arguement to kernel
    ciErrNum  = clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void *) &cl_generators);  //colors is second arguement to kernel
    ciErrNum  = clSetKernelArg(ckKernel, 3, sizeof(cl_mem), (void *) &cl_velocities);  //colors is second arguement to kernel
    ciErrNum  = clSetKernelArg(ckKernel, 4, sizeof(cl_mem), (void *) &cl_life);  //colors is second arguement to kernel
    printf("done with popCorn()\n");

}


int EnjaParticles::init_cl()
{
    setup_cl();
   
    cqCommandQueue = clCreateCommandQueue(cxGPUContext, cdDevices[uiDeviceUsed], 0, &ciErrNum);
    //shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

    // Program Setup
    int pl;
    size_t program_length;
    printf("open the program\n");
    
    //CL_SOURCE_DIR is set in the CMakeLists.txt
    std::string path(CL_SOURCE_DIR);
    path += programs[system];
    printf("%s\n", path.c_str());
    char* cSourceCL = file_contents(path.c_str(), &pl);
    //printf("file: %s\n", cSourceCL);
    program_length = (size_t)pl;

    // create the program
    cpProgram = clCreateProgramWithSource(cxGPUContext, 1,
                      (const char **) &cSourceCL, &program_length, &ciErrNum);

    printf("building the program\n");
    // build the program
    ciErrNum = clBuildProgram(cpProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
    //ciErrNum = clBuildProgram(cpProgram, 0, NULL, NULL, NULL, NULL);
	if(ciErrNum != CL_SUCCESS){
		cl_build_status build_status;
		ciErrNum = clGetProgramBuildInfo(cpProgram, cdDevices[uiDeviceUsed], CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);

		char *build_log;
		size_t ret_val_size;
		ciErrNum = clGetProgramBuildInfo(cpProgram, cdDevices[uiDeviceUsed], CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);

		build_log = new char[ret_val_size+1];
		ciErrNum = clGetProgramBuildInfo(cpProgram, cdDevices[uiDeviceUsed], CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
		build_log[ret_val_size] = '\0';
		printf("BUILD LOG: \n %s", build_log);
	}
    printf("program built\n");
    ckKernel = clCreateKernel(cpProgram, "enja", &ciErrNum);
    printf("kernel made: %s\n", oclErrorString(ciErrNum));


    ts_cl[0] = new GE::Time("acquire", 5);
    ts_cl[1] = new GE::Time("ndrange", 5);
    ts_cl[2] = new GE::Time("release", 5);
    ts_cl[3] = new GE::Time("glFinish", 5);

    popCorn();

    return 1;
}




int EnjaParticles::setup_cl()
{
    //setup devices and context
    szGlobalWorkSize[0] = num; //set the workgroup size to number of particles

    cl_int ciErrNum;
    //Get the NVIDIA platform
    ciErrNum = oclGetPlatformID(&cpPlatform);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

    // Get the number of GPU devices available to the platform
    ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 0, NULL, &uiDevCount);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

    // Create the device list
    cdDevices = new cl_device_id [uiDevCount];
    ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, uiDevCount, cdDevices, NULL);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

    // Get device requested on command line, if any
    uiDeviceUsed = 0;
    unsigned int uiEndDev = uiDevCount - 1;

    bool bSharingSupported = false;
    for(unsigned int i = uiDeviceUsed; (!bSharingSupported && (i <= uiEndDev)); ++i) 
    {
        size_t extensionSize;
        ciErrNum = clGetDeviceInfo(cdDevices[i], CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize );
        //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
        if(extensionSize > 0) 
        {
            char* extensions = (char*)malloc(extensionSize);
            ciErrNum = clGetDeviceInfo(cdDevices[i], CL_DEVICE_EXTENSIONS, extensionSize, extensions, &extensionSize);
            //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
            std::string stdDevString(extensions);
            free(extensions);

            size_t szOldPos = 0;
            size_t szSpacePos = stdDevString.find(' ', szOldPos); // extensions string is space delimited
            while (szSpacePos != stdDevString.npos)
            {
                if( strcmp(GL_SHARING_EXTENSION, stdDevString.substr(szOldPos, szSpacePos - szOldPos).c_str()) == 0 ) 
                {
                    // Device supports context sharing with OpenGL
                    uiDeviceUsed = i;
                    bSharingSupported = true;
                    break;
                }
                do 
                {
                    szOldPos = szSpacePos + 1;
                    szSpacePos = stdDevString.find(' ', szOldPos);
                } 
                while (szSpacePos == szOldPos);
            }
        }
    }

    // Define OS-specific context properties and create the OpenCL context
    //#if defined (__APPLE_CC__)
    #if defined (__APPLE__) || defined(MACOSX)
        CGLContextObj kCGLContext = CGLGetCurrentContext();
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
        cl_context_properties props[] =
        {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
            0
        };
        cxGPUContext = clCreateContext(props, 0,0, NULL, NULL, &ciErrNum);
    #else
        #if defined WIN32 // Win32
            cl_context_properties props[] = 
            {
                CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), 
                CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), 
                CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform, 
                0
            };
            cxGPUContext = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &ciErrNum);
        #else
            cl_context_properties props[] = 
            {
                CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
                CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
                CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform, 
                0
            };
            cxGPUContext = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &ciErrNum);
        #endif
    #endif
    //shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

    // Log device used (reconciled for requested requested and/or CL-GL interop capable devices, as applies)
    //shrLog("Device # %u, ", uiDeviceUsed);
    //oclPrintDevName(LOGBOTH, cdDevices[uiDeviceUsed]);
 
}
*/
