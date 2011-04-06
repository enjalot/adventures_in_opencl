#ifndef RADIXSORT_PARENT_HPP
#define RADIXSORT_PARENT_HPP

#include <vector>
#include <cmath>


namespace amd
{
    class RadixSort
    {
    protected:
        int _sortSize;
        int _numKeyBits;
        int _numIssueGroups;
        int _scanBlockSize;
        int _numScanGroups;

        const int _groupSize;
        const int _elementsPerWorkItem;
        /** Set to true if the intermediate buffer has been initialized */
        bool _intermediateBufferInitialized;

        std::vector< std::string > _kernelNames;

        virtual void initializeAcceleratorEnvironment() = 0;
        virtual void initializeAcceleratorBuffers( int elementCount, int numBlocks, int numScanGroups ) = 0;
        virtual void initializeAcceleratorDataBuffer( int elementCount, int numBlocks, int numScanGroups ) = 0;
        virtual void initializeAcceleratorKernels() = 0;

        bool _environmentInitialised;

    public:

        RadixSort( ) :
            _sortSize( 0 ),
            _numKeyBits( 32 ),
            _numIssueGroups( 0 ),
            _scanBlockSize( SCAN_WORKGROUP_SIZE*2 ),
            _numScanGroups( 0 ),
            _groupSize( GROUP_SIZE ),
            _elementsPerWorkItem( ELEMENTS_PER_WORK_ITEM ),
            _intermediateBufferInitialized( false ),
            _environmentInitialised( false )
        {
            _kernelNames.push_back("RadixSortLocal");
            _kernelNames.push_back("ScanLargeArrays");
            _kernelNames.push_back("ScanPropagateBlockSums");
            _kernelNames.push_back("RadixSortGlobal");
            _kernelNames.push_back("SumBlockSums");
        }

        virtual ~RadixSort()
        {
        }

        /**
         * Wait for completion of the sort operation.
         */
        virtual void wait() = 0;
    };

} // namespace amd


#endif // #ifndef RADIXSORT_PARENT_HPP
