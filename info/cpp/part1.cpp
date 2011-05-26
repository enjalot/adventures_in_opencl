#include <stdio.h>

#include "cll.h"
#include "util.h"

void CL::popCorn()
{
    printf("in popCorn\n");

    //initialize our kernel from the program
    kernel = clCreateKernel(program, "part1", &err);
    printf("clCreateKernel: %s\n", oclErrorString(err));

    //initialize our CPU memory arrays, send them to the device and set the kernel arguements
    num = 10;
    float *a = new float[num];
    float *b = new float[num];
    for(int i=0; i < num; i++)
    {
        a[i] = 1.0f * i;
        b[i] = 1.0f * i;
    }

    printf("Creating OpenCL arrays\n");
    //our input arrays
    //create our OpenCL buffer for a, copying the data from CPU to the GPU at the same time
    cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float) * num, a, &err);
    //cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float) * num, b, &err);
    //we could do b similar, but you may want to create your buffer and fill it at a different time
    cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * num, NULL, &err);
    //our output array
    cl_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * num, NULL, &err);

    printf("Pushing data to the GPU\n");
    //push our CPU arrays to the GPU
//    err = clEnqueueWriteBuffer(command_queue, cl_a, CL_TRUE, 0, sizeof(float) * num, a, 0, NULL, &event);
//    clReleaseEvent(event); //we need to release events in order to be completely clean (has to do with openclprof)
//
    //push b's data to the GPU
    err = clEnqueueWriteBuffer(command_queue, cl_b, CL_TRUE, 0, sizeof(float) * num, b, 0, NULL, &event);
    clReleaseEvent(event);
    

    //set the arguements of our kernel
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &cl_a);
    err  = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &cl_b);
    err  = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &cl_c);
    //Wait for the command queue to finish these commands before proceeding
    clFinish(command_queue);

    //clean up allocated space.
    delete[] a;
    delete[] b;

    //for now we make the workgroup size the same as the number of elements in our arrays
    workGroupSize[0] = num;
}

void CL::runKernel()
{
    printf("in runKernel\n");
    //execute the kernel
    err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, workGroupSize, NULL, 0, NULL, &event);
    clReleaseEvent(event);
    printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
    clFinish(command_queue);

    //lets check our calculations by reading from the device memory and printing out the results
    float c_done[num];
    err = clEnqueueReadBuffer(command_queue, cl_c, CL_TRUE, 0, sizeof(float) * num, &c_done, 0, NULL, &event);
    printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));
    clReleaseEvent(event);

    for(int i=0; i < num; i++)
    {
        printf("c_done[%d] = %g\n", i, c_done[i]);
    }
}


