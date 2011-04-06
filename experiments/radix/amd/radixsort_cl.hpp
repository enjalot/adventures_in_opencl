#ifndef RADIXSORT_CL_HPP
#define RADIXSORT_CL_HPP

#include "radixsort_parent.hpp"

// Kernels included as string in this header
#include "radixsort_cl_kernels.hpp"

// OpenCL headers
#if defined(__APPLE__) || defined(__MACOSX)
# include <OpenCL/cl.hpp>
#else // !__APPLE__
# include <CL/cl.hpp>
#endif // !__APPLE__

#include <iostream>
#include <map>

namespace amd
{
    class RadixSortCL : public RadixSort
    {
    protected:
        // Map of kernel name to descriptor
        std::map<std::string, cl::Kernel> kernels;

        /** Intermediate/scratch buffer */
        cl::Buffer _scratchBuffer;


        // Local histogram buffer that stays local
        cl::Buffer localHistogramsBuffer;
        // Radix major layout local hist buffer that will be scanned, before and after
        // (these can probably be merged to do a scan-in-place
        cl::Buffer localHistogramsBufferGlobalSummed;
        cl::Buffer localHistogramsBufferGlobalSummedPostScan;
        cl::Buffer scanIntermediateBlockSumBuffer;

        cl::Event _event;
        cl::Context context;
        cl::CommandQueue queue;
        cl::Program program;
        cl::Program::Binaries binaries;

        cl_int status;
        bool _profile;


        /*
         * \brief OpenCL related initialisations are done here.
         *        Context, Device list, Command Queue are set up.
         *        Calls are made to set up OpenCL memory buffers that this program uses
         *        and to load the programs into memory and get kernel handles.
         */
        virtual void
        initializeAcceleratorEnvironment()
        {

            // Create and build CL program and load all kernels
            std::string kernelString( clKernelsString );

            std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
            if (devices.size() == 0) {
                std::cerr << "No device available\n";
                exit(1);
            }

            cl::Program::Sources source(
                    1,
                    std::make_pair(
                            kernelString.c_str(),
                            kernelString.length()));
            program = cl::Program(context, source);
            cl_int err = program.build(devices, "-I./");
            if (err != CL_SUCCESS) {
                std::cerr << "Program::build() failed (" << err << ")\n";
                std::string log;
                program.getBuildInfo(devices[0], CL_PROGRAM_BUILD_LOG, &log);
                std::cerr << log.c_str() << "\n";
                exit(1);
            }

        } // initialiseAcceleratorEnvironment


        /*
         * \brief Set up OpenCL memory buffers used by this program
         */
        virtual void
        initializeAcceleratorBuffers(int elementCount, int numBlocks, int numScanGroups)
        {
            cl_int error;

            int clFlags = 0;
            localHistogramsBuffer = cl::Buffer(
                       context,
                       clFlags,
                       (1<<BITS_PER_PASS)*numBlocks*sizeof(uint32_t),
                       NULL,
                       &error);
            if(error != CL_SUCCESS)
            {
                std::cout<<"Could not create local histograms buffer.\n";
                exit(1);
            }

            clFlags = 0;
            localHistogramsBufferGlobalSummed = cl::Buffer(
                       context,
                       clFlags,
                       (1<<BITS_PER_PASS)*numBlocks*sizeof(uint32_t),
                       NULL,
                       &error);
            if(error != CL_SUCCESS)
            {
                std::cout<<"Could not create local summed histograms buffer.\n";
                exit(1);
            }

            clFlags = 0;
            localHistogramsBufferGlobalSummedPostScan = cl::Buffer(
                       context,
                       clFlags,
                       (1<<BITS_PER_PASS)*numBlocks*sizeof(uint32_t),
                       NULL,
                       &error);
            if(error != CL_SUCCESS)
            {
                std::cout<<"Could not create globally summed local histograms buffer.\n";
                exit(1);
            }

            clFlags = 0;
            scanIntermediateBlockSumBuffer = cl::Buffer(
                       context,
                       clFlags,
                       numScanGroups*sizeof(KeyValuePair),
                       NULL,
                       &error);

            if(error != CL_SUCCESS)
            {
                std::cout<<"Could not create internediate block sum buffer with error: " << error << ".\n";
                exit(1);
            }
        } // initialiseAcceleratorDataBuffers


        /*
         * \brief Load and build OpenCL program and get kernel handles
         */
        virtual void
        initializeAcceleratorKernels()
        {
            cl_int err;
            for(unsigned int i = 0; i < _kernelNames.size(); i++) {
                kernels[_kernelNames[i]] = cl::Kernel(
                            program,
                            _kernelNames[i].c_str(),
                            &err);
                if (err != CL_SUCCESS) {
                    std::cerr << "Kernel::Kernel() failed (" << err << ") for kernel " << _kernelNames[i].c_str() << "\n";
                    exit(1);
                }
            }
        }  // initialiseAcceleratorKernels

        /*
         * \brief Set up intermediate OpenCL buffer that will be used for computation.
         */
        virtual void
        initializeAcceleratorDataBuffer(int elementCount, int numBlocks, int numScanGroups)
        {
            cl_int error;

            int clFlags = 0;
            _scratchBuffer = cl::Buffer(
                       context,
                       clFlags,
                       elementCount*sizeof(KeyValuePair),
                       NULL,
                       &error);
            if(error != CL_SUCCESS)
            {
                std::cout<<"Could not create data buffer 1.\n";
                exit(1);
            }

            _intermediateBufferInitialized = true;
        } // initialiseAcceleratorDataBuffer


        /**
         * Performs a prefix sum of inputScanArray into outputScanArray using scanBlockSums as an intermediate
         * buffer for the sums of individual work groups during the process.
         *
         * Current version uses the CPU to do block sums as it's a simple operation but this has excessive overhead, really.
         * The propagation of block sums is done on the GPU, but really everything should be.
         * For very large arrays the block sum computation will need a tree of blocks rather than a single block with a two
         * step global process, so that should be done carefully eventually.
         * End event is the class variable _event.
         */
        void
        scanLargeArrays(
                cl::Kernel &scanLargeArraysKernel,
                cl::Kernel &sumBlockSumsKernel,
                cl::Kernel &scanPropagateBlockSumsKernel,
                VECTOR_CLASS<cl::Event> &sortStartEvents )
        {
            {
                cl_int status;


                // Total work item count so multiply number of groups by items per group
                cl::NDRange global(
                        _numScanGroups*SCAN_WORKGROUP_SIZE,
                        1,
                        1);
                cl::NDRange local(SCAN_WORKGROUP_SIZE, 1, 1);

                status = queue.enqueueNDRangeKernel(
                        scanLargeArraysKernel,
                        cl::NullRange,
                        global,
                        local,
                        &sortStartEvents,
                        &_event);
                if(status!= CL_SUCCESS)
                {
                    std::cout << "Kernel ScanLargeArrays failed with error: " << status << ".\n";
                    exit(1);
                }

                if ( _profile ) {
                    _event.wait();
                    cl_long submit = _event.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
                    cl_long start = _event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
                    cl_long end = _event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                    std::cout << "Elapsed time for local scan kernel execution: " << (double)(end - start) / 1e6 << "ms and from submit to end: " << (double)(end - submit) / 1e6  << "\n";
                }

                // Reprepare sort event structure
                sortStartEvents.clear();
                sortStartEvents.push_back( _event );
            }



            {
                cl_int status;


                // Total work item count so multiply number of groups by items per group
                cl::NDRange global(
                    BLOCK_SUM_WORKGROUP_SIZE,
                    1,
                    1);
                cl::NDRange local(BLOCK_SUM_WORKGROUP_SIZE, 1, 1);



                status = queue.enqueueNDRangeKernel(
                        sumBlockSumsKernel,
                        cl::NullRange,
                        global,
                        local,
                        &sortStartEvents,
                        &_event);
                if(status!= CL_SUCCESS)
                {
                    std::cout << "Kernel SumBlockSums failed with error: " << status << ".\n";
                    exit(1);
                }

                if ( _profile ) {
                    _event.wait();
                    cl_long submit = _event.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
                    cl_long start = _event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
                    cl_long end = _event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                                std::cout << "Elapsed time for block sum kernel execution: " << (double)(end - start) / 1e6 << "ms and from submit to end: " << (double)(end - submit) / 1e6  << "\n";
                }

                // Reprepare sort event structure
                sortStartEvents.clear();
                sortStartEvents.push_back( _event );
            }


            ///////////
            // Propagate block sum data through the main array
            // no need to do anything if there is only 1 group because we need not propagate 0s into it
            if( _numScanGroups > 1 )
            {
                // Total work item count so multiply number of groups by items per group
                // One fewer groups for propagation than for scan because we don't need to propagate 0s into the first group
                cl::NDRange global(
                    (_numScanGroups-1)*SCAN_WORKGROUP_SIZE,
                    1,
                    1);
                cl::NDRange local(SCAN_WORKGROUP_SIZE, 1, 1);

                cl_int status;

                status = queue.enqueueNDRangeKernel(
                        scanPropagateBlockSumsKernel,
                        cl::NullRange,
                        global,
                        local,
                        &sortStartEvents,
                        &_event);
                if(status!= CL_SUCCESS)
                {
                    std::cout << "Kernel ScanPropagateBlockSums failed with error: " << status << ".\n";
                    exit(1);
                }
                if ( _profile ) {
                    _event.wait();
                    cl_long submit = _event.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
                    cl_long start = _event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
                    cl_long end = _event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                    std::cout << "Elapsed time for sum propagation kernel execution: " << (double)(end - start) / 1e6 << "ms and from submit to end: " << (double)(end - submit) / 1e6  << "\n";
                }

                // Reprepare sort event structure
                sortStartEvents.clear();
                sortStartEvents.push_back( _event );
            }
        } // scanLargeArrays


        /*
         * \brief The internal sort function.
         * Data assumed to be in the correct start buffer to end up in outputBuffer at the end.
         */
        void
        sortInternal(
                cl::Buffer &scratchBuffer,
                cl::Buffer &outputBuffer,
                bool oddPassCount,
                VECTOR_CLASS<cl::Event> &sortStartEvents )
        {
            cl_int error = 0;

            // Correctly prepare the ping-pong buffers
            cl::Buffer buffer1;
            cl::Buffer buffer2;
            if( oddPassCount )
            {
                buffer1 = scratchBuffer;
                buffer2 = outputBuffer;
            } else {
                buffer1 = outputBuffer;
                buffer2 = scratchBuffer;
            }

            unsigned elementsPerBlock = _groupSize*_elementsPerWorkItem;

            // Total work item count so multiply number of groups by items per group
            cl::NDRange global( _numIssueGroups*_groupSize, 1, 1);
            cl::NDRange local(_groupSize, 1, 1);

            using std::max;
            // Local memory storage
            unsigned int totalLocalMemoryWords =
                elementsPerBlock + 16;


            // Set static kernel arguments out of main loop
            cl::Kernel &radixSortLocalKernel( kernels["RadixSortLocal"] );
            radixSortLocalKernel.setArg(1, localHistogramsBufferGlobalSummed);
            radixSortLocalKernel.setArg(2, localHistogramsBuffer);
            radixSortLocalKernel.setArg(4, _numIssueGroups);
            // *2 for initial counters and then for prefix sums
            // maybe that can compress into the same array
            radixSortLocalKernel.setArg(5, cl::__local(totalLocalMemoryWords * sizeof(uint32_t)));

            cl::Kernel &globalSortKernel(kernels["RadixSortGlobal"]);
            globalSortKernel.setArg(1, localHistogramsBufferGlobalSummedPostScan);
            globalSortKernel.setArg(2, localHistogramsBuffer);
            globalSortKernel.setArg(5, cl::__local(3*(1<<BITS_PER_PASS)* sizeof(uint32_t)));


            cl::Kernel &scanLargeArraysKernel( kernels["ScanLargeArrays"] );
            scanLargeArraysKernel.setArg(0, localHistogramsBufferGlobalSummedPostScan);
            scanLargeArraysKernel.setArg(1, localHistogramsBufferGlobalSummed);
            scanLargeArraysKernel.setArg(2, cl::__local(_scanBlockSize * sizeof(uint32_t)));
            scanLargeArraysKernel.setArg(3, _scanBlockSize);
            scanLargeArraysKernel.setArg(4, _numIssueGroups*(1<<BITS_PER_PASS));
            scanLargeArraysKernel.setArg(5, scanIntermediateBlockSumBuffer);

            cl::Kernel &sumBlockSumsKernel( kernels["SumBlockSums"] );
            sumBlockSumsKernel.setArg(0, scanIntermediateBlockSumBuffer);
            sumBlockSumsKernel.setArg(1, _numScanGroups);
            // TODO: This number's a bit random. Need to make it based off something
            sumBlockSumsKernel.setArg(2, cl::__local(2048));

            cl::Kernel &scanPropagateBlockSumsKernel( kernels["ScanPropagateBlockSums"] );
            scanPropagateBlockSumsKernel.setArg(0, localHistogramsBufferGlobalSummedPostScan);
            scanPropagateBlockSumsKernel.setArg(1, _scanBlockSize);
            scanPropagateBlockSumsKernel.setArg(2, _numIssueGroups*(1<<BITS_PER_PASS));
            scanPropagateBlockSumsKernel.setArg(3, scanIntermediateBlockSumBuffer);


            // Main loop over sets of bits
            for( int bitIndex = 0; bitIndex < _numKeyBits; bitIndex += BITS_PER_PASS  )
            {
                if ( _profile )
                  std::cout << "Bit index: " << bitIndex << "\n";

                // Stage 1 and 2 from Satish paper:
                // Sort blocks of array in local memory of separate work groups
                // Output the sorted sublists and histograms representing the
                // number of each value in the sublist
                radixSortLocalKernel.setArg(0, buffer1);
                radixSortLocalKernel.setArg(3, bitIndex);


                status = queue.enqueueNDRangeKernel(
                        radixSortLocalKernel,
                          cl::NullRange,
                          global,
                          local,
                          &sortStartEvents,
                          &_event);
                if(status!= CL_SUCCESS)
                {
                    std::cout << "Kernel RadixSortLocal failed with error: " << status << ".\n";
                    exit(1);
                }

                // Reprepare sort event structure
                sortStartEvents.clear();
                sortStartEvents.push_back( _event );

                if ( _profile ) {
                    _event.wait();
                    cl_long submit = _event.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
                    cl_long start = _event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
                    cl_long end = _event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                    std::cout << "Elapsed time for local sort kernel execution: " << (double)(end - start) / 1e6 << "ms and from submit to end: " << (double)(end - submit) / 1e6  << "\n";
                }

                // State 3 from Satish paper:
                // Perform prefix sum over histogram bins
                scanLargeArrays(
                        scanLargeArraysKernel,
                        sumBlockSumsKernel,
                        scanPropagateBlockSumsKernel,
                        sortStartEvents
                        );

                // Stage 4 from Satish paper:
                // Do radix shuffle based on the prefix summed offsets
                globalSortKernel.setArg(0, buffer1);
                globalSortKernel.setArg(3, buffer2);
                globalSortKernel.setArg(4, bitIndex);

                // 2x because we're storing two histograms - local and global
                status = queue.enqueueNDRangeKernel(
                        globalSortKernel,
                        cl::NullRange,
                        global,
                        local,
                        &sortStartEvents,
                        &_event);
                if(status!= CL_SUCCESS)
                {
                    std::cout << "Kernel RadixSortGlobal failed with error: " << status << ".\n";
                    exit(1);
                }

                if ( _profile ) {
                    _event.wait();
                    cl_long submit = _event.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
                    cl_long start = _event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
                    cl_long end = _event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                    std::cout << "Elapsed time for global sort kernel execution: " << (double)(end - start) / 1e6 << "ms and from submit to end: " << (double)(end - submit) / 1e6  << "\n";
                }

                // Swap buffers
                cl::Buffer tempBuffer = buffer1;
                buffer1 = buffer2;
                buffer2 = tempBuffer;

                // Reprepare sort event structure for next iteration
                sortStartEvents.clear();
                sortStartEvents.push_back( _event );

            }
        } // sortInternal


    public:
        RadixSortCL( bool useGPU = true, bool profile = false ) :
            RadixSort(),
            _profile( profile )
        {

        }

        virtual ~RadixSortCL()
        {

        }


        /*
         * \brief OpenCL related initialisations are done here.
         *        Context, Device list, Command Queue are set up.
         *        Unlike the more general setup the user passes in preallocated buffers,
         *        assumed to be of elementCount*sizeof(uint32_t)
         *        initializeInternalBuffer - true if we want to create an internal scratch buffer on initialization
         */
        void initializeSort(
                const cl::Context& contextL,
                const cl::CommandQueue& queueL,
                int sortSize,
                int keySize = 32,
                bool initializeInternalBuffer = true)
        {
            _sortSize = sortSize;
            _numKeyBits = keySize;
            _numIssueGroups = ( (_sortSize / ELEMENTS_PER_WORK_ITEM)/GROUP_SIZE );
            _numScanGroups = (int)(std::ceil(float(_numIssueGroups)*(1<<BITS_PER_PASS)/_scanBlockSize));

            if( !_environmentInitialised )
            {
                context = contextL;
                queue   = queueL;
                initializeAcceleratorEnvironment();
                initializeAcceleratorKernels();
                _environmentInitialised = true;
            }

            initializeAcceleratorBuffers( _sortSize, _numIssueGroups, _numScanGroups );

            // Create the internal data buffer if requested
            if( initializeInternalBuffer )
                initializeAcceleratorDataBuffer( _sortSize, _numIssueGroups, _numScanGroups );
        } // initialiseSortWithBuffers

        /**
         * Sort inputBuffer into outputBuffer.
         * If mutableInputBuffer set then the inputBuffer is allowed to be used as scratch.
         * If not an internal scratch buffer will be used (and created if not already).
         */
        void
        sort( cl::Buffer &inputBuffer, cl::Buffer &outputBuffer, bool mutableInputBuffer, const VECTOR_CLASS<cl::Event> &events = VECTOR_CLASS<cl::Event>() )
        {
            cl::Event returnEvent;
            sort( inputBuffer, outputBuffer, mutableInputBuffer, returnEvent, events );
        }

        /**
         * Sort inputBuffer into outputBuffer.
         * If mutableInputBuffer set then the inputBuffer is allowed to be used as scratch.
         * If not an internal scratch buffer will be used (and created if not already).
         */
        void
        sort( cl::Buffer &inputBuffer, cl::Buffer &outputBuffer, bool mutableInputBuffer, cl::Event &endEvent, const VECTOR_CLASS<cl::Event> &events = VECTOR_CLASS<cl::Event>() )
        {
            int numPasses = _numKeyBits/BITS_PER_PASS;
            bool oddPassCount = numPasses & 1;
            bool requireScratchBuffer = (numPasses>1) && !mutableInputBuffer;


            cl_int status;

            VECTOR_CLASS<cl::Event> sortStartEvents;

            sortStartEvents = events;

            // If intermediate scratch buffer hasn't already been initialized, create it
            if( requireScratchBuffer && !_intermediateBufferInitialized )
                initializeAcceleratorDataBuffer( _sortSize, _numIssueGroups, _numScanGroups );


            if( requireScratchBuffer )
            {
                // If we need a scratch buffer then copy out of the input buffer to start
                // and use the scratch buffer in the sort

                // If we have an even number of passes start in the output buffer, otherwise start in the scratch buffer
                // Start with copy to initialize this
                if( oddPassCount )
                {
                    // Enqueue a copy and make its event the one necessary for the sort
                    status = queue.enqueueCopyBuffer( inputBuffer, _scratchBuffer, 0, 0, _sortSize*sizeof(KeyValuePair), &sortStartEvents, &_event );
                    if(status != CL_SUCCESS)
                    {
                        std::cout<<"Could not read from element buffer\n.";
                        exit(1);
                    }

                    // Reprepare sort event structure
                    sortStartEvents.clear();
                    sortStartEvents.push_back( _event );
                } else {
                    // Enqueue a copy and make its event the one necessary for the sort
                    status = queue.enqueueCopyBuffer( inputBuffer, outputBuffer, 0, 0, _sortSize*sizeof(KeyValuePair), &sortStartEvents, &_event );
                    if(status != CL_SUCCESS)
                    {
                        std::cout<<"Could not read from element buffer\n.";
                        exit(1);
                    }

                    // Reprepare sort event structure
                    sortStartEvents.clear();
                    sortStartEvents.push_back( _event );
                }

                sortInternal( _scratchBuffer, outputBuffer, oddPassCount, sortStartEvents );
            } else {
                // If we do not need a scratch buffer (input can be overwritten) use it.
                // Only copy out of it if we have an even number of passes.

                if( !oddPassCount )
                {
                    // Enqueue a copy and make its event the one necessary for the sort
                    status = queue.enqueueCopyBuffer( inputBuffer, outputBuffer, 0, 0, _sortSize*sizeof(KeyValuePair), &sortStartEvents, &_event );
                    if(status != CL_SUCCESS)
                    {
                        std::cout<<"Could not read from element buffer\n.";
                        exit(1);
                    }

                    // Reprepare sort event structure
                    sortStartEvents.clear();
                    sortStartEvents.push_back( _event );
                }

                sortInternal( inputBuffer, outputBuffer, oddPassCount, sortStartEvents );
            }



            // Return the termination event to the caller
            endEvent = _event;
        }

        void
        sort( cl::Buffer &IOBuffer, const VECTOR_CLASS<cl::Event> &events = VECTOR_CLASS<cl::Event>() )
        {
            cl::Event returnEvent;
            sort( IOBuffer, returnEvent, events );
        }

        void
        sort( cl::Buffer &IOBuffer, cl::Event &endEvent , const VECTOR_CLASS<cl::Event> &events = VECTOR_CLASS<cl::Event>())
        {
            int numPasses = _numKeyBits/BITS_PER_PASS;
            bool oddPassCount = numPasses & 1;

            // Always need a scratch buffer with only one input buffer
            if( !_intermediateBufferInitialized )
                initializeAcceleratorDataBuffer( _sortSize, _numIssueGroups, _numScanGroups );

            cl_int status;

            VECTOR_CLASS<cl::Event> sortStartEvents( events );

            // If we have an odd number of passes, then we must copy the data into the scratch buffer to start
            if( oddPassCount )
            {
                // Enqueue a copy and make its event the one necessary for the sort
                status = queue.enqueueCopyBuffer( IOBuffer, _scratchBuffer, 0, 0, _sortSize*sizeof(KeyValuePair), &sortStartEvents, &_event );
                if(status != CL_SUCCESS)
                {
                    std::cout<<"Could not read from element buffer\n.";
                    exit(1);
                }

                // Reprepare sort event structure
                sortStartEvents.clear();
                sortStartEvents.push_back( _event );
            }

            sortInternal( _scratchBuffer, IOBuffer, oddPassCount, sortStartEvents );

            // Return the termination event to the caller
            endEvent = _event;
        }




        /**
         * Wait for completion of last operation.
         * Blocks until the sort is completed.
         */
        virtual void
        wait()
        {
            _event.wait();
        } // wait
    }; // class RadixSort
}


#endif // #ifndef RADIXSORT_CL_HPP
