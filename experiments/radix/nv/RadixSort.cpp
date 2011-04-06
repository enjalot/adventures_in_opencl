/*
* Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property and 
* proprietary rights in and to this software and related documentation.
* Any use, reproduction, disclosure, or distribution of this software
* and related documentation without an express license agreement from 
* NVIDIA Corporation is strictly prohibited.
* 
* Please refer to the applicable NVIDIA end user license agreement (EULA) 
* associated with this source code for terms and conditions that govern 
* your use of this NVIDIA software.
* 
*/

#include <stdio.h>
//#include <oclUtils.h>
#include "RadixSort.h"
#include <vector>
#include <string>
using namespace std;

extern double time1, time2, time3, time4;

//----------------------------------------------------------------------
void RadixSort::printInts(cl_mem d_var, int nb_el, const char* msg)
{
    vector<int> h_mem(nb_el);
	printf("h_mem size: %d\n", h_mem.size());
	clEnqueueReadBuffer(cqCommandQueue, d_var, CL_TRUE, 0, sizeof(unsigned int) * nb_el, &h_mem[0], 0, NULL, NULL);
	for (int i=0; i < 10; i++) {
		printf("%s[%d]: %d\n", msg, i, h_mem[i]);
	}
}
//----------------------------------------------------------------------
#if 0
void RadixSort::printInts4(cl_mem d_var, int nb_el, const char* msg)
{
    vector<int> h_mem(nb_el);
	printf("h_mem size: %d\n", h_mem.size());
	clEnqueueReadBuffer(cqCommandQueue, d_var, CL_TRUE, 0, 4*sizeof(unsigned int) * nb_el, &h_mem[0], 0, NULL, NULL);
	for (int i=0; i < 10; i++) {
		printf("%s: %d, %d, %d, %d, %e\n", msg, i, h_mem[i]);
	}
}
#endif
//----------------------------------------------------------------------
RadixSort::RadixSort(cl_context GPUContext,
					 cl_command_queue CommandQue,
					 unsigned int maxElements, 
					 const char* path, 
					 const int ctaSize,
					 bool keysOnly) :
					 mNumElements(0),
					 mTempValues(0),
					 mCounters(0),
					 mCountersSum(0),
					 mBlockOffsets(0),
					 cxGPUContext(GPUContext),
					 cqCommandQueue(CommandQue),
					 CTA_SIZE(ctaSize),
					 scan(GPUContext, CommandQue, maxElements/2/CTA_SIZE*16, path)
{

	unsigned int numBlocks = ((maxElements % (CTA_SIZE * 4)) == 0) ? 
            (maxElements / (CTA_SIZE * 4)) : (maxElements / (CTA_SIZE * 4) + 1);
    unsigned int numBlocks2 = ((maxElements % (CTA_SIZE * 2)) == 0) ?
            (maxElements / (CTA_SIZE * 2)) : (maxElements / (CTA_SIZE * 2) + 1);

	cl_int ciErrNum;
	d_tempKeys = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, sizeof(unsigned int) * maxElements, NULL, &ciErrNum);
	// Not sure this is required (G. Erlebacher, 9/11/2010)
	d_tempValues = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, sizeof(unsigned int) * maxElements, NULL, &ciErrNum);
	mCounters = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, WARP_SIZE * numBlocks * sizeof(unsigned int), NULL, &ciErrNum);
	mCountersSum = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, WARP_SIZE * numBlocks * sizeof(unsigned int), NULL, &ciErrNum);
	mBlockOffsets = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, WARP_SIZE * numBlocks * sizeof(unsigned int), NULL, &ciErrNum); 

	size_t szKernelLength; // Byte size of kernel code

	string paths(CL_SORT_SOURCE_DIR);
	paths = paths + "/RadixSort.cl";
	const char* pathr = paths.c_str();
	FILE* fd =fopen(pathr, "r");
	printf("fd= %d\n", fd);
	char* cRadixSort = new char [30000];
	int nb = fread(cRadixSort, 1, 30000, fd);
	printf("pathr= %s\n", pathr);
    szKernelLength = nb;


    //oclCheckErrorEX(cRadixSort == NULL, false, NULL);
    cpProgram = clCreateProgramWithSource(cxGPUContext, 1, (const char **)&cRadixSort, &szKernelLength, &ciErrNum);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
#ifdef MAC
    char *flags = "-DMAC -cl-fast-relaxed-math";
#else
    char *flags = "-cl-fast-relaxed-math";
#endif
    ciErrNum = clBuildProgram(cpProgram, 0, NULL, flags, NULL, NULL);
    if (ciErrNum != CL_SUCCESS)
    {
        // write out standard ciErrNumor, Build Log and PTX, then cleanup and exit
         /*
        shrLogEx(LOGBOTH | ERRORMSG, ciErrNum, STDERROR);
        oclLogBuildInfo(cpProgram, oclGetFirstDev(cxGPUContext));
        oclLogPtx(cpProgram, oclGetFirstDev(cxGPUContext), "RadixSort.ptx");
        oclCheckError(ciErrNum, CL_SUCCESS); 
        */
    }

	//ckRadixSortBlocksKeysOnly = clCreateKernel(cpProgram, "radixSortBlocksKeysOnly", &ciErrNum);
	ckRadixSortBlocksKeysValues = clCreateKernel(cpProgram, "radixSortBlocksKeysValues", &ciErrNum);
	//oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
	ckFindRadixOffsets        = clCreateKernel(cpProgram, "findRadixOffsets",        &ciErrNum);
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
	ckScanNaive               = clCreateKernel(cpProgram, "scanNaive",               &ciErrNum);
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
	ckReorderDataKeysValues     = clCreateKernel(cpProgram, "reorderDataKeysValues",     &ciErrNum);
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
	free(cRadixSort);
}

RadixSort::~RadixSort()
{
	//clReleaseKernel(ckRadixSortBlocksKeysOnly);
	clReleaseKernel(ckRadixSortBlocksKeysValues);
	clReleaseKernel(ckFindRadixOffsets);
	clReleaseKernel(ckScanNaive);
	//clReleaseKernel(ckReorderDataKeysOnly);
	clReleaseKernel(ckReorderDataKeysValues);
	clReleaseProgram(cpProgram);

	clReleaseMemObject(d_tempKeys);
	clReleaseMemObject(d_tempValues);
	clReleaseMemObject(mCounters);
	clReleaseMemObject(mCountersSum);
	clReleaseMemObject(mBlockOffsets);
}

//------------------------------------------------------------------------
// Sorts input arrays of unsigned integer keys and (optional) values
// 
// @param d_keys      Array of keys for data to be sorted
// @param values      Array of values to be sorted
// @param numElements Number of elements to be sorted.  Must be <= 
//                    maxElements passed to the constructor
// @param keyBits     The number of bits in each key to use for ordering
//------------------------------------------------------------------------
void RadixSort::sort(cl_mem d_keys, 
		  cl_mem d_values,
		  unsigned int  numElements,
		  unsigned int  keyBits)
{
	//radixSortKeysOnly(d_keys, numElements, keyBits);
	radixSortKeysValues(d_keys, d_values, numElements, keyBits);
	//printInts(d_keys, 10, "keys");
	//printInts(d_values, 10, "values");
}

//----------------------------------------------------------------------------
// Main key-only radix sort function.  Sorts in place in the keys and values 
// arrays, but uses the other device arrays as temporary storage.  All pointer 
// parameters are device pointers.  Uses cudppScan() for the prefix sum of
// radix counters.
//----------------------------------------------------------------------------
#if 0
void RadixSort::radixSortKeysOnly(cl_mem d_keys, unsigned int numElements, unsigned int keyBits)
{
	int i = 0;
    while (keyBits > i*bitStep) 
	{
		radixSortStepKeysOnly(d_keys, bitStep, i*bitStep, numElements);
		i++;
	}
}
#endif

// Added by G. Erlebacher 9/11/2010
void RadixSort::radixSortKeysValues(cl_mem d_keys, cl_mem d_values, unsigned int numElements, unsigned int keyBits)
{
	//printf("enter radixSortKeysValues\n");
	int i = 0;
    while (keyBits > i*bitStep) 
	{
		//radixSortStepKeysOnly(d_keys, bitStep, i*bitStep, numElements);
		radixSortStepKeysValues(d_keys, d_values, bitStep, i*bitStep, numElements);
		i++;
	}
}

//----------------------------------------------------------------------------
// Perform one step of the radix sort.  Sorts by nbits key bits per step, 
// starting at startbit.
//----------------------------------------------------------------------------
#if 0
void RadixSort::radixSortStepKeysOnly(cl_mem d_keys, unsigned int nbits, unsigned int startbit, unsigned int numElements)
{
	// Four step algorithms from Satish, Harris & Garland
	radixSortBlocksKeysOnlyOCL(d_keys, nbits, startbit, numElements);

	findRadixOffsetsOCL(startbit, numElements);

	scan.scanExclusiveLarge(mCountersSum, mCounters, 1, numElements/2/CTA_SIZE*16);

	reorderDataKeysOnlyOCL(d_keys, startbit, numElements);
}
#endif

//----------------------------------------------------------------------
// Added by G. Erlebacher, 9/11/2010
void RadixSort::radixSortStepKeysValues(cl_mem d_keys, cl_mem d_values, unsigned int nbits, unsigned int startbit, unsigned int numElements)
{
	//printf("enter radixSortStepKeysValues\n");
	// Four step algorithms from Satish, Harris & Garland
	//radixSortBlocksKeysOnlyOCL(d_keys, nbits, startbit, numElements);
	radixSortBlocksKeysValuesOCL(d_keys, d_values, nbits, startbit, numElements);
//exit(3);

	findRadixOffsetsOCL(startbit, numElements);

	//printf("numElements= %d\n", numElements);
	//printf("CTA_SIZE=%d\n", CTA_SIZE);
	scan.scanExclusiveLarge(mCountersSum, mCounters, 1, numElements/2/CTA_SIZE*16);

	//reorderDataKeysOnlyOCL(d_keys, startbit, numElements);
	reorderDataKeysValuesOCL(d_keys, d_values, startbit, numElements);
}
//----------------------------------------------------------------------------
// Wrapper for the kernels of the four steps
//----------------------------------------------------------------------------
#if 0
void RadixSort::radixSortBlocksKeysOnlyOCL(cl_mem d_keys, unsigned int nbits, unsigned int startbit, unsigned int numElements)
{
	unsigned int totalBlocks = numElements/4/CTA_SIZE;
	size_t globalWorkSize[1] = {CTA_SIZE*totalBlocks};
	size_t localWorkSize[1] = {CTA_SIZE};
	cl_int ciErrNum;
	ciErrNum  = clSetKernelArg(ckRadixSortBlocksKeysOnly, 0, sizeof(cl_mem), (void*)&d_keys);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysOnly, 1, sizeof(cl_mem), (void*)&d_tempKeys);
	ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysOnly, 2, sizeof(unsigned int), (void*)&nbits);
	ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysOnly, 3, sizeof(unsigned int), (void*)&startbit);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysOnly, 4, sizeof(unsigned int), (void*)&numElements);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysOnly, 5, sizeof(unsigned int), (void*)&totalBlocks);
	ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysOnly, 6, 4*CTA_SIZE*sizeof(unsigned int), NULL);
    ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckRadixSortBlocksKeysOnly, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
}
#endif

void RadixSort::radixSortBlocksKeysValuesOCL(cl_mem d_keys, cl_mem d_values, unsigned int nbits, unsigned int startbit, unsigned int numElements)
{
	//printf("enter radixSortBlocks\n");
	unsigned int totalBlocks = numElements/4/CTA_SIZE;
	size_t globalWorkSize[1] = {CTA_SIZE*totalBlocks};
	size_t localWorkSize[1] = {CTA_SIZE};
	cl_int ciErrNum;
	ciErrNum  = clSetKernelArg(ckRadixSortBlocksKeysValues, 0, sizeof(cl_mem), (void*)&d_keys);
	ciErrNum  = clSetKernelArg(ckRadixSortBlocksKeysValues, 1, sizeof(cl_mem), (void*)&d_values);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 2, sizeof(cl_mem), (void*)&d_tempKeys);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 3, sizeof(cl_mem), (void*)&d_tempValues);
	ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 4, sizeof(unsigned int), (void*)&nbits);
	ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 5, sizeof(unsigned int), (void*)&startbit);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 6, sizeof(unsigned int), (void*)&numElements);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 7, sizeof(unsigned int), (void*)&totalBlocks);
	ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 8, 4*CTA_SIZE*sizeof(unsigned int), NULL);
    ciErrNum |= clSetKernelArg(ckRadixSortBlocksKeysValues, 9, 4*CTA_SIZE*sizeof(unsigned int), NULL);
    ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckRadixSortBlocksKeysValues, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);

	// ERROR MESSGE: ERror #-33 (CL_INVALID_DEVICE)   WHY??????
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);


	//printf("-----------------------------------------------\n");
	//printInts(d_keys, 12, "d_keys(radixSortBlocksKeysOnlyOCL)");
	//printInts(d_values, 12, "d_values(radixSortBlocksKeysOnlyOCL)");
	//printInts(d_tempKeys, 12, "d_tempKeys(radixSortBlocksKeysOnlyOCL)");
	//printInts(d_tempValues, 12, "d_tempValues(radixSortBlocksKeysOnlyOCL)");
	//printf("-----------------------------------------------\n");
}


//----------------------------------------------------------------------
void RadixSort::findRadixOffsetsOCL(unsigned int startbit, unsigned int numElements)
{
	unsigned int totalBlocks = numElements/2/CTA_SIZE;
	size_t globalWorkSize[1] = {CTA_SIZE*totalBlocks};
	size_t localWorkSize[1] = {CTA_SIZE};
	cl_int ciErrNum;
	ciErrNum  = clSetKernelArg(ckFindRadixOffsets, 0, sizeof(cl_mem), (void*)&d_tempKeys);
	ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 1, sizeof(cl_mem), (void*)&d_tempValues);
	ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 2, sizeof(cl_mem), (void*)&mCounters);
    ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 3, sizeof(cl_mem), (void*)&mBlockOffsets);
	ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 4, sizeof(unsigned int), (void*)&startbit);
	ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 5, sizeof(unsigned int), (void*)&numElements);
	ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 6, sizeof(unsigned int), (void*)&totalBlocks);
	ciErrNum |= clSetKernelArg(ckFindRadixOffsets, 7, 2 * CTA_SIZE * sizeof(unsigned int), NULL);
	ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckFindRadixOffsets, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
}

#define NUM_BANKS 16
void RadixSort::scanNaiveOCL(unsigned int numElements)
{
	unsigned int nHist = numElements/2/CTA_SIZE*16;
	size_t globalWorkSize[1] = {nHist};
	size_t localWorkSize[1] = {nHist};
	unsigned int extra_space = nHist / NUM_BANKS;
	unsigned int shared_mem_size = sizeof(unsigned int) * (nHist + extra_space);
	cl_int ciErrNum;
	ciErrNum  = clSetKernelArg(ckScanNaive, 0, sizeof(cl_mem), (void*)&mCountersSum);
	ciErrNum |= clSetKernelArg(ckScanNaive, 1, sizeof(cl_mem), (void*)&mCounters);
	ciErrNum |= clSetKernelArg(ckScanNaive, 2, sizeof(unsigned int), (void*)&nHist);
	ciErrNum |= clSetKernelArg(ckScanNaive, 3, 2 * shared_mem_size, NULL);
	ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckScanNaive, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
}

#if 0
void RadixSort::reorderDataKeysOnlyOCL(cl_mem d_keys, unsigned int startbit, unsigned int numElements)
{
	unsigned int totalBlocks = numElements/2/CTA_SIZE;
	size_t globalWorkSize[1] = {CTA_SIZE*totalBlocks};
	size_t localWorkSize[1] = {CTA_SIZE};
	cl_int ciErrNum;
	ciErrNum  = clSetKernelArg(ckReorderDataKeysOnly, 0, sizeof(cl_mem), (void*)&d_keys);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 1, sizeof(cl_mem), (void*)&d_tempKeys);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 2, sizeof(cl_mem), (void*)&mBlockOffsets);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 3, sizeof(cl_mem), (void*)&mCountersSum);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 4, sizeof(cl_mem), (void*)&mCounters);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 5, sizeof(unsigned int), (void*)&startbit);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 6, sizeof(unsigned int), (void*)&numElements);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 7, sizeof(unsigned int), (void*)&totalBlocks);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysOnly, 8, 2 * CTA_SIZE * sizeof(unsigned int), NULL);
	ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckReorderDataKeysOnly, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
}
#endif

void RadixSort::reorderDataKeysValuesOCL(cl_mem d_keys, cl_mem d_values, unsigned int startbit, unsigned int numElements)
{
	//printf("enter reorder\n");
	unsigned int totalBlocks = numElements/2/CTA_SIZE;
	size_t globalWorkSize[1] = {CTA_SIZE*totalBlocks};
	size_t localWorkSize[1] = {CTA_SIZE};
	cl_int ciErrNum;
	ciErrNum  = clSetKernelArg(ckReorderDataKeysValues, 0, sizeof(cl_mem), 		(void*)&d_keys);
	#if 1
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 1, sizeof(cl_mem), 		(void*)&d_values);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 2, sizeof(cl_mem), 		(void*)&d_tempKeys);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 3, sizeof(cl_mem), 		(void*)&d_tempValues);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 4, sizeof(cl_mem), 		(void*)&mBlockOffsets);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 5, sizeof(cl_mem), 		(void*)&mCountersSum);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 6, sizeof(cl_mem), 		(void*)&mCounters);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 7, sizeof(unsigned int), 	(void*)&startbit);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 8, sizeof(unsigned int), 	(void*)&numElements);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 9, sizeof(unsigned int), 	(void*)&totalBlocks);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 10, 2 * CTA_SIZE * sizeof(unsigned int), NULL);
	ciErrNum |= clSetKernelArg(ckReorderDataKeysValues, 11, 2 * CTA_SIZE * sizeof(unsigned int), NULL);
	#endif
	ciErrNum |= clEnqueueNDRangeKernel(cqCommandQueue, ckReorderDataKeysValues, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	////oclCheckErrorEX(ciErrNum, CL_SUCCESS, NULL);
}
