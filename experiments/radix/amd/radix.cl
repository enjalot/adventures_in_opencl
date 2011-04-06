//////////////////////////////////////////////////////////////////////////////////////
// Private constants
typedef uint2 KeyValuePair;



//#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


// May want to factor BITS_PER_PASS out
#define BITS_PER_PASS 4

#define GROUP_SIZE 128
#define ELEMENTS_PER_WORK_ITEM 4
#define SCAN_WORKGROUP_SIZE 128
#define BLOCK_SUM_WORKGROUP_SIZE 128
#define NUM_BLOCK_SUM_ELEMENTS_PER_ITEM ((GRID_SIZE+255)/BLOCK_SUM_WORKGROUP_SIZE)


#define LOG_NUM_BANKS 5
#define NUM_LOCAL_BANKS (1<<LOG_NUM_BANKS)


// Remove the top one and replace with the others, then can put the tuned versions back in
#if 1
#define CONVERT_CONFLICT_FREE(in) (in)
#define CONVERT_CONFLICT_FREE_2VEC(in) (in)
#define CONVERT_CONFLICT_FREE_4VEC(in) (in)
#else
#define CONVERT_CONFLICT_FREE(in) (in + (in>>LOG_NUM_BANKS))
#define CONVERT_CONFLICT_FREE_2VEC(in) (intermediate.x = in.x>>LOG_NUM_BANKS, intermediate.y = in.y>>LOG_NUM_BANKS, in + intermediate)
#define CONVERT_CONFLICT_FREE_4VEC(in) (intermediate.x = in.x>>LOG_NUM_BANKS, intermediate.y = in.y>>LOG_NUM_BANKS, intermediate.z = in.z>>LOG_NUM_BANKS, intermediate.w = in.w>>LOG_NUM_BANKS, in + intermediate)
#endif

#define BANK_CONFLICT_RESOLUTION_PADDING (CONVERT_CONFLICT_FREE(GROUP_SIZE*ELEMENTS_PER_WORK_ITEM)-GROUP_SIZE*ELEMENTS_PER_WORK_ITEM)




////////////////////////////////////////////////////////////////////////////////////////
// Prefixsum functions

/**
 * Perform work efficient prefix sum in local memory in place.
 */
void localPrefixSum(  __local unsigned *prefixSums, unsigned numElements )
{
    // Iterate over halving sizes of the element set performing reduction phase of scan

    int offset = 1;
    //for( int level = get_local_size(0); level > 0; level >>= 1 )
    for( int level = numElements>>1; level > 0; level >>= 1 )
    //for( int level = 1; level > 0; level >>= 1 )
    {
        barrier(CLK_LOCAL_MEM_FENCE);

        for( int sumElement = get_local_id(0); sumElement < level; sumElement += get_local_size(0) )
        {
            //int ai = offset*(2*sumElement+1)-1;
            //int bi = offset*(2*sumElement+2)-1;

            int i = 2*offset*sumElement;
            int ai = i + offset - 1;
            int bi = ai + offset;

            ai = CONVERT_CONFLICT_FREE(ai);
            bi = CONVERT_CONFLICT_FREE(bi);
            prefixSums[bi] += prefixSums[ai];
        }
        offset <<= 1;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // Need to clear the last element
    if( get_local_id(0) == 0 )
        prefixSums[ CONVERT_CONFLICT_FREE(numElements-1) ] = 0;

    for( int level = 1; level < numElements; level <<= 1 )
    {
        offset >>= 1;
        barrier(CLK_LOCAL_MEM_FENCE);

        for( int sumElement = get_local_id(0); sumElement < level; sumElement += get_local_size(0) )
        {
            int ai = offset*(2*sumElement+1)-1;
            int bi = offset*(2*sumElement+2)-1;

            ai = CONVERT_CONFLICT_FREE(ai);
            bi = CONVERT_CONFLICT_FREE(bi);
            unsigned temporary = prefixSums[ai];
            prefixSums[ai] = prefixSums[bi];
            prefixSums[bi] += temporary;
        }
    }
}

// Barrier free because we know that this never gets wider than a WF
void BClocalPF(  __local unsigned *prefixSums )
{
    // Iterate over halving sizes of the element set performing reduction phase of scan
    int offset = 1;

    for( int level = 8; level > 0; level >>= 1 )
    {
        for( int sumElement = get_local_id(0); sumElement < level; sumElement += get_local_size(0) )
        {
            int ai = offset*(2*sumElement+1)-1;
            int bi = offset*(2*sumElement+2)-1;
            prefixSums[bi] += prefixSums[ai];
        }
        offset <<= 1;
    }


    // Need to clear the last element
    if( get_local_id(0) == 0 )
        prefixSums[ 15 ] = 0;

    for( int level = 1; level < 16; level <<= 1 )
    {
        offset >>= 1;

        for( int sumElement = get_local_id(0); sumElement < level; sumElement += get_local_size(0) )
        {
            int ai = offset*(2*sumElement+1)-1;
            int bi = offset*(2*sumElement+2)-1;
            unsigned temporary = prefixSums[ai];
            prefixSums[ai] = prefixSums[bi];
            prefixSums[bi] += temporary;
        }
    }
}



/**
 * Optimised prefix sum that takes 4 elements per WI and assumes 128 WIs in
 * a group.
 * Also depends on WF size of 64 elements. Will need minor adaptation to scale down.
 */
uint4 localPrefixSumBlock( uint4 prefixSumData, __local unsigned *prefixSums )
{
    uint4 originalData = prefixSumData;

    // Do sum across vector in two stages
    prefixSumData.y += prefixSumData.x;
    prefixSumData.w += prefixSumData.z;

    prefixSumData.z += prefixSumData.y;
    prefixSumData.w += prefixSumData.y;

    // Now just 128 values, each sum of a block of 4
    prefixSums[get_local_id(0)] = 0;
    prefixSums[get_local_id(0)+128] = prefixSumData.w;

    // TODO: Could get rid of these barriers if get both WFs working but on different halves of data
    barrier(CLK_LOCAL_MEM_FENCE);

    // Do for single WF as we only have 128 values to deal with = 64*2
/*
	//	lh.todo. th. doesn't work on v2.2 because compiler mess this up. Update of prefixSums[idx] is only done at the end of this {}.
	//	1. orig ver
    if( get_local_id(0) < 64 )
    {
        int idx = 2*get_local_id(0) + 129;
        prefixSums[idx] += prefixSums[idx-1];
        prefixSums[idx] += prefixSums[idx-2];
        prefixSums[idx] += prefixSums[idx-4];
        prefixSums[idx] += prefixSums[idx-8];
        prefixSums[idx] += prefixSums[idx-16];
        prefixSums[idx] += prefixSums[idx-32];
        prefixSums[idx] += prefixSums[idx-64];
        // Propagate intermediate values through
        prefixSums[idx-1] += prefixSums[idx-2];
    }
*/
	//	2. using mem fence
    if( get_local_id(0) < 64 )
    {
        int idx = 2*get_local_id(0) + 129;
        prefixSums[idx] += prefixSums[idx-1];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        prefixSums[idx] += prefixSums[idx-2];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        prefixSums[idx] += prefixSums[idx-4];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        prefixSums[idx] += prefixSums[idx-8];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        prefixSums[idx] += prefixSums[idx-16];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        prefixSums[idx] += prefixSums[idx-32];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        prefixSums[idx] += prefixSums[idx-64];
		mem_fence(CLK_LOCAL_MEM_FENCE);
        // Propagate intermediate values through
        prefixSums[idx-1] += prefixSums[idx-2];
    }
/*
	//	this should work on v2.2 too. Checked assembly but doesnt run as I expect.
	//	3. using volatile (doesn't work somehow)
	volatile __local unsigned* ps = prefixSums;
    if( get_local_id(0) < 64 )
    {
        int idx = 2*get_local_id(0) + 129;
        ps[idx] += ps[idx-1];
        ps[idx] += ps[idx-2];
        ps[idx] += ps[idx-4];
        ps[idx] += ps[idx-8];
        ps[idx] += ps[idx-16];
        ps[idx] += ps[idx-32];
        ps[idx] += ps[idx-64];
        // Propagate intermediate values through
        ps[idx-1] += ps[idx-2];
    }
*/
/*
	//	4. using barrier
    {
        int idx = 2*get_local_id(0) + 129;
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-1];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-2];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-4];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-8];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-16];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-32];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx] += prefixSums[idx-64];
        // Propagate intermediate values through
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	    if( get_local_id(0) < 64 )
		{
	        prefixSums[idx-1] += prefixSums[idx-2];
		}
    }
*/

    barrier(CLK_LOCAL_MEM_FENCE);

    // Grab and propagate for whole WG - loading the - 1 value
    uint addValue = prefixSums[get_local_id(0)+127];

    // Propagate item level sum across vector
    prefixSumData += (uint4)(addValue, addValue, addValue, addValue);

    // And return the final value which is the final sum
    return prefixSumData - originalData;
}












////////////////////////////////////////////////////////////////////////////////////////
// Public functions



/**
 * Generate a local histogram from the sorted data counting how many entries of each set of 4 local digits we have.
 * This version just uses atomics to do summation because the number should be relatively small.
 *
 * alternative: Histogram has 32(local mem banks)*2^numBits entries which are reduced into the first 2^numBits elements in this function.
 * The rest of the histogram array is temporary data.
 * There are 32 histogram bins for each value to avoid the need for atomics.
 *
 * @param histogramOutputRadixMajor is ordered by radices - inefficient read and write but easily prefix summed.
 * This will be the global set of offsets.
 * @param histogramOutputGroupMajor is ordered by groups. This is the local set of offsets for the radix sort scatter to use
 * later that will not be globally summed.
 */
void generateHistogram(
		uint4 sortedData,
		__local unsigned *histogram,
		__global unsigned *histogramOutputRadixMajor,
		__global unsigned *histogramOutputGroupMajor,
		unsigned startBit,
        unsigned numGroups)
{
    uint4 addresses;
    addresses = (uint4)(get_local_id(0), get_local_id(0), get_local_id(0), get_local_id(0));
    // This bit is best unvectorised as we can't write more than 2 values to local per WI anyway
    if( get_local_id(0) < (1<<BITS_PER_PASS) )
    {
    	histogram[addresses.x] = 0;
    }

    addresses = addresses * (unsigned)4;
    addresses.y = addresses.y+1;
    addresses.z = addresses.z+2;
    addresses.w = addresses.w+3;

	// Obtain correct histogram address using only the BITS_PER_PASS bits that we're sorting by in this pass
	sortedData.x >>= startBit;
	sortedData.y >>= startBit;
	sortedData.z >>= startBit;
	sortedData.w >>= startBit;

	int andValue = ((1<<BITS_PER_PASS)-1);
	sortedData &= (uint4)(andValue, andValue, andValue, andValue);

	// Alternative approach counting differences
	// Doesn't work yet, and will probably never be faster anyway due to ifs
	// only makes sense if atomics are *very* slow
#if 0
	histogram[16+get_local_id(0)] = sortedData.w;
#endif

    // Perform the atomic histogram updates
	barrier(CLK_LOCAL_MEM_FENCE);

	// Alternative approach counting differences
	// Doesn't work yet, and will probably never be faster anyway due to ifs
	// only makes sense if atomics are *very* slow
#if 0
	uint previousValue = histogram[16-1+get_local_id(0)];
	if(sortedData.x != previousValue && get_local_id(0) > 0)
		histogram[previousValue] = get_local_id(0)*4;
	if(sortedData.z != sortedData.y)
		histogram[sortedData.y] = get_local_id(0)*4;
	if(sortedData.w != sortedData.z)
		histogram[sortedData.z] = get_local_id(0)*4;
#else
	atomic_inc( &(histogram[sortedData.x]) );
	atomic_inc( &(histogram[sortedData.y]) );
	atomic_inc( &(histogram[sortedData.z]) );
	atomic_inc( &(histogram[sortedData.w]) );
#endif

	barrier(CLK_LOCAL_MEM_FENCE);


    // No need to vectorise this with only 16 values to process
    if( get_local_id(0) < 16 )
    {
     	uint histValues;

     	histValues = histogram[get_local_id(0)];

     	unsigned globalOffset = 16*get_group_id(0);
     	uint globalAddresses = get_local_id(0) + globalOffset;

     	uint globalAddressesRadixMajor = numGroups;
     	globalAddressesRadixMajor = globalAddressesRadixMajor * get_local_id(0);
     	globalAddressesRadixMajor = globalAddressesRadixMajor + get_group_id(0);


     	histogramOutputGroupMajor[globalAddresses] = histValues;
     	histogramOutputRadixMajor[globalAddressesRadixMajor] = histValues;
     }
}



#define LOCAL_SORT_WG_SIZE 128

/*
 * Perform radix sort operation on a local array of unsigned ints
 * based on a particular set of bit entries of BITS_PER_PASS in size at
 * startbit in the data.
 * Outputs the data and a histogram representing the counts of all
 * BITS_PER_PASS sized values within the local memory array.
 * histogramOutput is stored as all the 0 bins, then all the 1 bins
 * and so on. Inefficient write out in here - simple prefix sum.
 */
__attribute__((reqd_work_group_size(LOCAL_SORT_WG_SIZE,1,1)))
__kernel void RadixSortLocal(
        // uint8 - this is really 4 KeyValuePairs packed together
		// There is probably a cleaner way to represent this
		__global uint8 *dataToSort,

		__global unsigned *histogramOutputGlobalRadixMajor,
		__global unsigned *histogramOutputLocalGroupMajor,
		int startBit,
        int numGroups,
		__local unsigned *sorterSharedMemory
        )
{

	int numLocalElements = get_local_size(0)*ELEMENTS_PER_WORK_ITEM;

    uint4 plainLocalAddress = (uint4)(get_local_id(0), get_local_id(0), get_local_id(0), get_local_id(0));
    uint4 addValues = (uint4)(0,1,2,3);
    plainLocalAddress = plainLocalAddress * (unsigned)4;
    plainLocalAddress = plainLocalAddress + addValues;

    // localAddress that's been converted to avoid conflicts
    uint4 localAddress = CONVERT_CONFLICT_FREE_4VEC(plainLocalAddress);

    uint4 localKeys;
    uint4 localValues;
	{
        uint4 globalAddress =  (uint4)(get_group_id(0), get_group_id(0), get_group_id(0), get_group_id(0));
        uint4 localElementsCount = (uint4)(numLocalElements, numLocalElements, numLocalElements, numLocalElements);
        globalAddress = globalAddress*localElementsCount+localAddress;

        // uint8 - this is really 4 KeyValuePairs packed together
        // Probably a cleaner way to represent this
        uint8 localData;
		localData = dataToSort[globalAddress.x/4];
		localKeys.s0 = localData.s0;
		localKeys.s1 = localData.s2;
		localKeys.s2 = localData.s4;
		localKeys.s3 = localData.s6;
		localValues.s0 = localData.s1;
		localValues.s1 = localData.s3;
		localValues.s2 = localData.s5;
		localValues.s3 = localData.s7;
	}

	// Iterate over the block of bits we are sorting internally in this kernel
	//for( int bitIndex = startBit; bitIndex < (startBit+BITS_PER_PASS ); ++bitIndex )
	int bitIndex = startBit;
	do
	{
		// Write a local mem value just after the range to be processed by the prefix sum

		// TODO: 512??
        if( get_local_id(0) == (get_local_size(0)-1) )
        	sorterSharedMemory[256] = localKeys.w;

        unsigned compare = (1<<bitIndex);
        uint4 compareVec = (uint4)(compare, compare, compare, compare);

        uint4 localCompareVec = localKeys & compareVec;

        uint4 prefixSum;
        prefixSum = select( (uint4)(1,1,1,1), (uint4)(0,0,0,0), localCompareVec != (uint4)(0,0,0,0) );

        prefixSum = localPrefixSumBlock(prefixSum, sorterSharedMemory);

        // Need to get totalFalses from somewhere - that's the value in [255]. Could just assume 255 of sorterSharedMemory is ok, but
        // that's hardly clean
        uint totalFalses = sorterSharedMemory[255];

		/////////////////
		// Perform the local sort
        // Rearrange data using local memory
        {
        	uint4 localCompareVec = localKeys & compareVec;

        	uint4 newAddress = plainLocalAddress - prefixSum;
        	newAddress += (uint4)(totalFalses, totalFalses, totalFalses, totalFalses);

        	newAddress = select( prefixSum, newAddress, localCompareVec != (uint4)(0, 0, 0, 0) );

        	newAddress = CONVERT_CONFLICT_FREE_4VEC(newAddress);

        	// Internally sort keys and then values
        	// May be more efficient to do this in one go using the pairs
        	barrier(CLK_LOCAL_MEM_FENCE);

        	sorterSharedMemory[newAddress.x] = localKeys.x;
        	sorterSharedMemory[newAddress.y] = localKeys.y;
        	sorterSharedMemory[newAddress.z] = localKeys.z;
        	sorterSharedMemory[newAddress.w] = localKeys.w;

        	barrier(CLK_LOCAL_MEM_FENCE);

            // Back up sort values to give thread-local store of current state of local mem
        	localKeys.x = sorterSharedMemory[localAddress.x];
        	localKeys.y = sorterSharedMemory[localAddress.y];
        	localKeys.z = sorterSharedMemory[localAddress.z];
        	localKeys.w = sorterSharedMemory[localAddress.w];

    		barrier(CLK_LOCAL_MEM_FENCE);

            sorterSharedMemory[newAddress.x] = localValues.x;
            sorterSharedMemory[newAddress.y] = localValues.y;
            sorterSharedMemory[newAddress.z] = localValues.z;
            sorterSharedMemory[newAddress.w] = localValues.w;

            barrier(CLK_LOCAL_MEM_FENCE);

            // Back up sort values to give thread-local store of current state of local mem
            localValues.x = sorterSharedMemory[localAddress.x];
            localValues.y = sorterSharedMemory[localAddress.y];
            localValues.z = sorterSharedMemory[localAddress.z];
            localValues.w = sorterSharedMemory[localAddress.w];

            barrier(CLK_LOCAL_MEM_FENCE);
        }

		bitIndex = bitIndex + 1;
	} while( bitIndex < (startBit+BITS_PER_PASS ) );

	// Generate local histogram and output to global histogram storage array
	generateHistogram( localKeys, sorterSharedMemory, histogramOutputGlobalRadixMajor, histogramOutputLocalGroupMajor, startBit, numGroups );

	{
        uint4 globalAddress =  (uint4)(get_group_id(0), get_group_id(0), get_group_id(0), get_group_id(0));
        uint4 localElementsCount = (uint4)(numLocalElements, numLocalElements, numLocalElements, numLocalElements);
        globalAddress = globalAddress*localElementsCount+plainLocalAddress;

        uint8 localData;
        localData.s0 = localKeys.s0;
        localData.s2 = localKeys.s1;
        localData.s4 = localKeys.s2;
        localData.s6 = localKeys.s3;
        localData.s1 = localValues.s0;
        localData.s3 = localValues.s1;
        localData.s5 = localValues.s2;
        localData.s7 = localValues.s3;

        // 2* because we have key/value pairs now
		dataToSort[globalAddress.x/4] = localData;
	}
}




/*
 * ScanLargeArrays : Scan is done for each block and the sum of each
 * block is stored in separate array (sumBuffer). SumBuffer is scanned
 * and results are added to every value of next corresponding block to
 * compute the scan of a large array.(not limited to 2*MAX_GROUP_SIZE)
 * Scan uses a balanced tree algorithm. See Belloch, 1990 "Prefix Sums
 * and Their Applications"
 * @param output output data
 * @param input  input data
 * @param block  local memory used in the kernel
 * @param length length of the input data
 * @param sumBuffer  sum of blocks
 */
__attribute__((reqd_work_group_size(128,1,1)))
__kernel
void ScanLargeArrays(
		__global unsigned int *output,
        __global unsigned int *input,
        __local  unsigned int *block,	 // Size : block_size
        const uint block_size,	 // size of block
        const uint length,	 	 // no of elements
        __global unsigned int *sumBuffer)  // sum of blocks

{
	int tid = get_local_id(0);
	int gid = get_global_id(0);
	int bid = get_group_id(0);

	int offset = 1;

    /* Cache the computational window in shared memory */
    if( (2*gid + 1) < length )
    {
	   block[2*tid]     = input[2*gid];
	   block[2*tid + 1] = input[2*gid + 1];
    }  else {
       block[2*tid]     = 0;
       block[2*tid + 1] = 0;
    }

    /* build the sum in place up the tree */
	for(int d = block_size>>1; d > 0; d >>=1)
	{
		barrier(CLK_LOCAL_MEM_FENCE);

		if(tid<d)
		{
			int ai = offset*(2*tid + 1) - 1;
			int bi = offset*(2*tid + 2) - 1;

			block[bi] += block[ai];
		}
		offset *= 2;
	}

	barrier(CLK_LOCAL_MEM_FENCE);


    /* store the value in sum buffer before making it to 0 */
	sumBuffer[bid] = block[block_size - 1];

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    /* scan back down the tree */

    /* clear the last element */
	block[block_size - 1] = 0;

    /* traverse down the tree building the scan in the place */
	for(int d = 1; d < block_size ; d *= 2)
	{
		offset >>=1;
		barrier(CLK_LOCAL_MEM_FENCE);

		if(tid < d)
		{
			int ai = offset*(2*tid + 1) - 1;
			int bi = offset*(2*tid + 2) - 1;

			unsigned int t = block[ai];
			block[ai] = block[bi];
			block[bi] += t;
		}
	}

	barrier(CLK_LOCAL_MEM_FENCE);

    /*write the results back to global memory */

	if( (2*gid + 1) < length )
    {
        output[2*gid]     = block[2*tid];
        output[2*gid + 1] = block[2*tid + 1];
    }

}

/**
  * ScanPropagateBlockSums : Takes as input blocks of data,
 * each individually prefix summed, as well as a set of block
 * sums that summarise the entire set.
 * Propagates the block sums into the prefix sum blocks.
 * @param scanArray the set of prefix summed blocks
 * @param block_size the number of data elements dealt with
 * by each work group. Need not relate directly to the
 * number of work items in a group, but should align for efficiency.
 * @param length length of the input data
 * @param sumBuffer  block sum array
 */
__attribute__((reqd_work_group_size(128,1,1)))
__kernel
void ScanPropagateBlockSums(
		__global unsigned int *scanArray,
        const uint block_size,	 // size of block
        const uint length,	 	 // no of elements
        __global unsigned int *sumBuffer)  // sum of blocks
{
	// Get the appropriate block sum for this group
	unsigned int blockSum = sumBuffer[get_group_id(0)+1];

	// For entire block, add the sum to it

	int endValue = min((get_group_id(0)+2)*(block_size), length);
	for(
			int i = (get_group_id(0)+1)*block_size + get_local_id(0);
			i < endValue;
			i += get_local_size(0))
	{
		scanArray[i] += blockSum;
	}
}

/**
 * Do a scan on the block sum values which can then be propagated back to the blocks.
 * Currently this does not deal correctly with non power-of-two sizes (I think
 * certainly some sizes of data set fail to sort correctly, problem could be elsewhere).
 */
__attribute__((reqd_work_group_size(128,1,1)))
__kernel
void SumBlockSums(__global unsigned *blockSums, unsigned numElements, __local unsigned *localMemory)
{
	int element;
	for( element = get_local_id(0); element < numElements; element += get_local_size(0) )
	{
		localMemory[CONVERT_CONFLICT_FREE(element)] = blockSums[element];
	}
/*
	for( ; element < 2048; element += get_local_size(0) )
	{
		localMemory[element + CONFLICT_FREE_OFFSET(element)] = 0;
	}*/

	barrier(CLK_LOCAL_MEM_FENCE);

	//localPrefixSum(localMemory, 2048);
	localPrefixSum(localMemory, numElements);

	barrier(CLK_LOCAL_MEM_FENCE);

	for( int element = get_local_id(0); element < (numElements); element += get_local_size(0) )
	{
		blockSums[element] = localMemory[CONVERT_CONFLICT_FREE(element)];
	}
}



/**
 * RadixSortGlobal: Perform the global radix sort scatter from
 * pre-sorted local buffers and both local and global prefix-sum
 * offset information.
 * @param dataToSort contains the buffer of locally sorted blocks
 * @param histogramGlobalRadixMajor is radix major arranged
 * global prefix sum offset information for the bit block values.
 * @param histogramLocalGroupMajor contains per-group
 * local prefix sum information.
 * @param startBit is the first of BITS_PER_PASS bits in the key this pass is
 * sorting by.
 * @param sorterSharedMemory is the shared memory storage region,
 * divided up as necessary.
 */
__attribute__((reqd_work_group_size(128,1,1)))
__kernel
void RadixSortGlobal(
        // uint8 - this is really 4 KeyValuePairs packed together
        // There is probably a cleaner way to represent this
		__global uint8 *dataToSort,

		__global unsigned *histogramGlobalRadixMajor,
		__global unsigned *histogramLocalGroupMajor,
		__global KeyValuePair *destinationArray,
		unsigned startBit,
		__local unsigned *sorterLocalMemory)
{
	// Move local histogram far enough through memory to allow for
	// efficient local prefix sum
	__local unsigned *localHistogram = sorterLocalMemory + 2*(1<<BITS_PER_PASS);
	__local unsigned *globalHistogram = sorterLocalMemory;

	// First load the local and global histogram data into local memory
	// And do prefix sum of local histogram
	// No point in vectorising this with only 16 bins
	if( get_local_id(0) < ((1<<BITS_PER_PASS)/2) )
	{
		uint2 histElement = (uint2)(get_local_id(0), get_local_id(0)+8);

		uint2 localValues;
		globalHistogram[histElement.x] = histogramGlobalRadixMajor[get_num_groups(0)*histElement.x + get_group_id(0)];
		globalHistogram[histElement.y] = histogramGlobalRadixMajor[get_num_groups(0)*histElement.y + get_group_id(0)];

		localValues.x = histogramLocalGroupMajor[(1<<BITS_PER_PASS)*get_group_id(0) + histElement.x];
		localValues.y = histogramLocalGroupMajor[(1<<BITS_PER_PASS)*get_group_id(0) + histElement.y];
		localHistogram[histElement.x] = localValues.x;
		localHistogram[histElement.y] = localValues.y;

		localHistogram[histElement.x-(1<<BITS_PER_PASS)] = 0;
		localHistogram[histElement.y-(1<<BITS_PER_PASS)] = 0;

		int idx = 2*get_local_id(0);
		localHistogram[idx] += localHistogram[idx-1];
		mem_fence(CLK_LOCAL_MEM_FENCE);
		localHistogram[idx] += localHistogram[idx-2];
		mem_fence(CLK_LOCAL_MEM_FENCE);
		localHistogram[idx] += localHistogram[idx-4];
		mem_fence(CLK_LOCAL_MEM_FENCE);
		localHistogram[idx] += localHistogram[idx-8];
		mem_fence(CLK_LOCAL_MEM_FENCE);

		// Propagate intermediate values through
		localHistogram[idx-1] += localHistogram[idx-2];
		mem_fence(CLK_LOCAL_MEM_FENCE);

		// Grab and propagate for whole WG - loading the - 1 value
		localValues.x = localHistogram[histElement.x-1];
		localValues.y = localHistogram[histElement.y-1];

		localHistogram[histElement.x] = localValues.x;
		localHistogram[histElement.y] = localValues.y;

	}


	barrier(CLK_LOCAL_MEM_FENCE);

    const int numLocalElements = 512;
    uint4 localAddress = (uint4)(get_local_id(0), get_local_id(0), get_local_id(0), get_local_id(0));
    localAddress = localAddress*(unsigned)4;
    uint4 addValues = (uint4)(0,1,2,3);
    localAddress = localAddress + addValues;
    uint4 globalAddress = get_group_id(0)*numLocalElements + localAddress;

    // uint8 - this is really 4 KeyValuePairs packed together
    // There is probably a cleaner way to represent this
	uint8 sortValue;
	sortValue = dataToSort[globalAddress.x/4];

	uint cmpValue = ((1<<BITS_PER_PASS)-1);
	uint4 cmpValueVector = (uint4)(cmpValue, cmpValue, cmpValue, cmpValue);
	uint4 radix;
	// s0,2,4,6 are keys, 1,3,5,7 are values
	radix.x = (sortValue.s0>>startBit);
	radix.y = (sortValue.s2>>startBit);
	radix.z = (sortValue.s4>>startBit);
	radix.w = (sortValue.s6>>startBit);

	radix = radix & cmpValueVector;

	uint4 localOffsetIntoRadixSet;
	localOffsetIntoRadixSet = localAddress;
	localOffsetIntoRadixSet.x = localOffsetIntoRadixSet.x - localHistogram[radix.x];
	localOffsetIntoRadixSet.y = localOffsetIntoRadixSet.y - localHistogram[radix.y];
	localOffsetIntoRadixSet.z = localOffsetIntoRadixSet.z - localHistogram[radix.z];
	localOffsetIntoRadixSet.w = localOffsetIntoRadixSet.w - localHistogram[radix.w];

	uint4 globalOffset = localOffsetIntoRadixSet;
	globalOffset.x = globalOffset.x + globalHistogram[radix.x];
	globalOffset.y = globalOffset.y + globalHistogram[radix.y];
	globalOffset.z = globalOffset.z + globalHistogram[radix.z];
	globalOffset.w = globalOffset.w + globalHistogram[radix.w];

	// s0,2,4,6 are keys, 1,3,5,7 are values
	destinationArray[globalOffset.x] = (KeyValuePair)(sortValue.s0, sortValue.s1);
	destinationArray[globalOffset.y] = (KeyValuePair)(sortValue.s2, sortValue.s3);
	destinationArray[globalOffset.z] = (KeyValuePair)(sortValue.s4, sortValue.s5);
	destinationArray[globalOffset.w] = (KeyValuePair)(sortValue.s6, sortValue.s7);
}

