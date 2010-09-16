#include <stdio.h>

#include "cll.h"
#include "util.h"

void CL::popCorn()
{
    printf("in popCorn\n");

    
    std::string a_path(CL_SOURCE_DIR); 
    a_path += "/a.cl"; 
    std::string b_path(CL_SOURCE_DIR);
    b_path += "/b.cl"; 
    a_program = loadProgram(a_path);
    b_program = loadProgram(b_path);

    //initialize our kernel from the program
    try{
        a_kernel = cl::Kernel(a_program, "a_kernel", &err);
        b_kernel = cl::Kernel(b_program, "b_kernel", &err);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%d)\n", er.what(), er.err());
    }

       

    //initialize our CPU memory arrays, send them to the device and set the kernel arguements
    num = 10;
    float *a = new float[num];
    float *b = new float[num];
    for(int i=0; i < num; i++)
    {
        a[i] = 1.0f * i;
        b[i] = 1.0f * i;
    }

    params.c1 = 2.0f;
    params.c2 = 3.0f;


    printf("Creating OpenCL arrays\n");
    size_t array_size = sizeof(float) * num;
    //our input arrays
    cl_a = cl::Buffer(context, CL_MEM_READ_ONLY, array_size, NULL, &err);
    cl_b = cl::Buffer(context, CL_MEM_READ_ONLY, array_size, NULL, &err);
    //our output array
    cl_c_a = cl::Buffer(context, CL_MEM_WRITE_ONLY, array_size, NULL, &err);
    cl_c_b = cl::Buffer(context, CL_MEM_WRITE_ONLY, array_size, NULL, &err);

    cl_params = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(Params), NULL, &err);

    printf("Pushing data to the GPU\n");
    //push our CPU arrays to the GPU
    err = queue.enqueueWriteBuffer(cl_a, CL_TRUE, 0, array_size, a, NULL, &event);
    err = queue.enqueueWriteBuffer(cl_b, CL_TRUE, 0, array_size, b, NULL, &event);
    
    //write the params struct to GPU memory as a buffer
    err = queue.enqueueWriteBuffer(cl_params, CL_TRUE, 0, sizeof(Params), &params, NULL, &event);
   


    //set the arguements of our kernel
    err = a_kernel.setArg(0, cl_a);
    err = a_kernel.setArg(1, cl_b);
    err = a_kernel.setArg(2, cl_c_a);
    err = a_kernel.setArg(3, cl_params);

    err = b_kernel.setArg(0, cl_a);
    err = b_kernel.setArg(1, cl_b);
    err = b_kernel.setArg(2, cl_c_b);
    err = b_kernel.setArg(3, cl_params);

    //Wait for the command queue to finish these commands before proceeding
    queue.finish();
 
    //for now we make the workgroup size the same as the number of elements in our arrays
    delete a;
    delete b;
}



void CL::runKernel()
{
    printf("in runKernel\n");
    //execute the kernel
    ///err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, workGroupSize, NULL, 0, NULL, &event);
    err = queue.enqueueNDRangeKernel(a_kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event); 
    err = queue.enqueueNDRangeKernel(b_kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event); 
    ///clReleaseEvent(event);
    printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
    ///clFinish(command_queue);
    queue.finish();

    //lets check our calculations by reading from the device memory and printing out the results
    float c_done[num];
    ///err = clEnqueueReadBuffer(command_queue, cl_c, CL_TRUE, 0, sizeof(float) * num, c_done, 0, NULL, &event);
    err = queue.enqueueReadBuffer(cl_c_a, CL_TRUE, 0, sizeof(float) * num, &c_done, NULL, &event);
    printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));
    //clReleaseEvent(event);

    for(int i=0; i < num; i++)
    {
        printf("c_done[%d] = %g\n", i, c_done[i]);
    }
}


