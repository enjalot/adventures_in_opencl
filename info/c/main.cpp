/*
 * Adventures in OpenCL tutorial series
 * Part 1
 *
 * author: Ian Johnson
 * code based on advisor Gordon Erlebacher's work
 * NVIDIA's examples
 * as well as various blogs and resources on the internet
 */
#include <stdio.h>
#include <util.h>

#if defined __APPLE__ || defined(MACOSX)
    #include <OpenCL/opencl.h>
#else
    #include <CL/opencl.h>
#endif


int main(int argc, char** argv)
{
    printf("Hello, OpenCL\n");

    //printf("oclGetPlatformID: %s\n", oclErrorString(err));


}
