#include <stdio.h>

#include "cll.h"
#include "util.h"



void CL::popCorn()
{
    printf("in popCorn\n");

    
    std::string path(CL_SOURCE_DIR);
    path += "/part3.cl"; 
    program = loadProgram(path);

    //initialize our kernel from the program
    try{
        kernel = cl::Kernel(program, "part3", &err);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%d)\n", er.what(), er.err());
    }

       

    //initialize our CPU memory arrays, send them to the device and set the kernel arguements
    num = 10;
    a.resize(num);
    b.resize(num);
    for(int i=0; i < num; i++)
    {
        a[i] = 1.0f;// * i;
        b[i] = 2.0f;// * i;
    }

    Params params;
    params.A = .5f;
    params.B = 10.0f;
    params.C = 3;

    printf("Creating OpenCL arrays\n");
    size_t array_size = sizeof(float) * num;
    //our input arrays
    cl_a = cl::Buffer(context, CL_MEM_READ_ONLY, array_size, NULL, &err);
    cl_b = cl::Buffer(context, CL_MEM_READ_ONLY, array_size, NULL, &err);
    //our output array
    cl_c = cl::Buffer(context, CL_MEM_WRITE_ONLY, array_size, NULL, &err);

    cl_params = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(Params), NULL, &err);

    printf("Pushing data to the GPU\n");
    //push our CPU arrays to the GPU
    //we can pass the address of the first element of our vector since it is a tightly packed array
    err = queue.enqueueWriteBuffer(cl_a, CL_TRUE, 0, array_size, &a[0], NULL, &event);
    err = queue.enqueueWriteBuffer(cl_b, CL_TRUE, 0, array_size, &b[0], NULL, &event);
    
    //write the params struct to GPU memory as a buffer
    err = queue.enqueueWriteBuffer(cl_params, CL_TRUE, 0, sizeof(Params), &params, NULL, &event);
   


    //set the arguements of our kernel
    err = kernel.setArg(0, cl_a);
    err = kernel.setArg(1, cl_b);
    err = kernel.setArg(2, cl_c);
    err = kernel.setArg(3, cl_params);

    //Wait for the command queue to finish these commands before proceeding
    queue.finish();
}



void CL::runKernel()
{
    printf("in runKernel\n");
    //execute the kernel
    ///err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, workGroupSize, NULL, 0, NULL, &event);
    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event); 
    ///clReleaseEvent(event);
    printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
    ///clFinish(command_queue);
    queue.finish();

    //lets check our calculations by reading from the device memory and printing out the results
    float c_gpu[num];

    err = queue.enqueueReadBuffer(cl_c, CL_TRUE, 0, sizeof(float) * num, &c_gpu, NULL, &event);
    printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));
    //clReleaseEvent(event);

    for(int i=0; i < num; i++)
    {
        printf("c_gpu[%d] = %g\n", 
                i, c_gpu[i]);
    }
}


