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
vector<cl::Context> contexts;

int main(int argc, char** argv)
{
    printf("Hello, OpenCL\n");

    /*
     * Get available platforms and display relevant information
     */
    try
    {
        //Get the available platforms on this machine
        cl::Error err = cl::Platform::get(&platforms);
    }
    catch (cl::Error er) { printf("ERROR: %s (%s)\n", er.what(), oclErrorString(er.err())); }

    printf("Number of platforms: %zd\n", platforms.size());
    //Print out some information about each of the platforms
    for(int i = 0; i < platforms.size(); i++)
    {
        string platform_name = platforms[i].getInfo<CL_PLATFORM_NAME>();
        string platform_vendor = platforms[i].getInfo<CL_PLATFORM_VENDOR>();
        string platform_extensions = platforms[i].getInfo<CL_PLATFORM_EXTENSIONS>();
        printf("Platform %d: %s by %s\n with extensions: %s\n", i,  platform_name.c_str(), 
                                                                    platform_vendor.c_str(), 
                                                                    platform_extensions.c_str());


        /*
         * Get the Devices available on this platform and print out information about them
         */
        vector<cl::Device> devices;
        platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);
        printf("Number of devices: %zd\n", devices.size());
        for(int j = 0; j < devices.size(); j++)
        {
            cl::Device d = devices[j];
            printf("====== Device %d ======\n", j);
            printf("Type: %s\n", GetCLPropertyString(d.getInfo<CL_DEVICE_TYPE>()) );
            printf("Name: %s\n", d.getInfo<CL_DEVICE_NAME>().c_str() );

            //Number of parallel compute cores on the device
            printf("Max Compute Units: %d\n", d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() );
            //Maximum dimensions that specify the global and local work-item
            //IDs used by the data parallel execution model
            printf("Max Dimensions: %d\n", d.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>() );
            printf("======================\n");
        }


        //This is the most basic set of properties supported by the standard
        //(specifying the platform) When doing OpenGL context sharing the
        //OpenGL context is passed in as a property (technically this is an
        //extension by the implementation)
        cl_context_properties properties[] = 
            { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[i])(), 0};

        cl::Context context = cl::Context(CL_DEVICE_TYPE_ALL, properties);

    }


}
