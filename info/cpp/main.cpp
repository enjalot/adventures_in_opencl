/*
 * Adventures in OpenCL tutorial series
 * Part 1
 *
 * author: Ian Johnson
 * code based on advisor Gordon Erlebacher's work
 * NVIDIA's examples
 * as well as various blogs and resources on the internet
 */
#define __CL_ENABLE_EXCEPTIONS

#include <stdio.h>
#include <vector>
#include "CL/cl.hpp"
#include "util.h"

using namespace std;

//Globally define all of our CL instances for convenience
vector<cl::Platform> platforms;


int main(int argc, char** argv)
{
    printf("Hello, OpenCL\n");

    try
    {
        //Get the available platforms on this machine
        cl::Error err = cl::Platform::get(&platforms);
    }
    catch (cl::Error er) { printf("ERROR: %s (%s)\n", er.what(), oclErrorString(er.err())); }

    printf("Number of platforms: %d\n", platforms.size());
    //Print out some information about each of the platforms
    for(int i = 0; i < platforms.size(); i++)
    {
        string platform_name = platforms[i].getInfo<CL_PLATFORM_NAME>();
        string platform_vendor = platforms[i].getInfo<CL_PLATFORM_VENDOR>();
        string platform_extensions = platforms[i].getInfo<CL_PLATFORM_EXTENSIONS>();
        printf("Platform %d: %s by %s\n with extensions:\n %s", i,  platform_name.c_str(), 
                                                                    platform_vendor.c_str(), 
                                                                    platform_extensions.c_str());
    }


}
