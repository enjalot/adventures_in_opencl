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
            //printf("Type: %s\n", GetCLPropertyString(d.getInfo<CL_DEVICE_TYPE>()) );
            const char* type = oclPropDevice(d.getInfo<CL_DEVICE_TYPE>());
            printf("====== Device %d (%s) ======\n", j, type);
            string name = d.getInfo<CL_DEVICE_NAME>();
            printf("  Name: %s\n", name.c_str());

            //Maximum dimensions that specify the global and local work-item
            //IDs used by the data parallel execution model
            int dimensions = d.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>(); 
            printf("  Max Dimensions: %d\n", dimensions);

            //Maximum work item sizes in each dimension
            vector<long unsigned int> max_wi_sizes = d.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
            printf("  Max Work Item Sizes: ");
            for(int k = 0; k < dimensions; k++)
                printf("%zd ", max_wi_sizes[k]);
            printf("\n");

            //Maximum work group size
            printf("  Max Work Group Size: %zd\n", d.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() );


            
            printf("  Memory:\n  ------\n");
            unsigned long global_size = (unsigned long)d.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
            printf("    Global Memory Size: %ld (bytes) or %4.0f (MB)\n", global_size, global_size / 1048576.0);
            unsigned long alloc_size = (unsigned long)d.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>(); 
            printf("    Max Mem Alloc Size: %ld (bytes) or %4.0f (MB)\n", alloc_size, alloc_size / 1048576.0);
            unsigned long cache_size = (unsigned long)d.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>(); 
            printf("    Global Memory Cache Size: %ld (bytes) or %4.0f (MB)\n", cache_size, cache_size / 1048576.0);
            printf("    Global Memory Cache Type: %s\n", oclPropCache(d.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_TYPE>()) );
            printf("\n");
            unsigned long local_size = (unsigned long)d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
            printf("    Local Memory Size: %ld (bytes) or %4.0f (KB)\n", local_size, local_size / 1024.0);
            printf("    Local Memory Type: %s\n", oclPropMem(d.getInfo<CL_DEVICE_LOCAL_MEM_TYPE>()) );
            printf("\n");
            unsigned long constant_size = (unsigned long)d.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
            printf("    Constant Memory Size: %ld (bytes) or %4.0f (KB)\n", constant_size, constant_size / 1024.0 );
            int constant_args = d.getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>();
            printf("    Max Constant Args: %d\n", constant_args);
            //1.1 only
            //printf("\n");
            //printf("  Unified Memory: %s\n", d.getInfo<CL_DEVICE_HOST_UNIFIED_MEMORY>() ? "True" : "False");
            printf("  ------\n");


            printf("  Image support: %s\n", d.getInfo<CL_DEVICE_IMAGE_SUPPORT>() ? "True" : "False");
            //Number of parallel compute cores on the device
            printf("  Max Compute Units: %d\n", d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() );
            printf("  Default Address Size: %dbit\n", (int)d.getInfo<CL_DEVICE_ADDRESS_BITS>() );
            printf("\n");
            //OpenCL version supported by the device
            const char* device_version = d.getInfo<CL_DEVICE_VERSION>().c_str();
            printf("  Device Version: %s\n", device_version); 
            const char* device_extensions = d.getInfo<CL_DEVICE_EXTENSIONS>().c_str();
            printf("  Device Extensions: %s\n", device_extensions);
            //1.1 only
            //const char* clc_version = d.getInfo<CL_DEVICE_OPENCL_C_VERSION>();
            //printf("  Device Version: %s\n", clc_version); 


            printf("======================\n\n");
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
