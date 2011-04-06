#ifndef RADIXSORT_HPP
#define RADIXSORT_HPP

// Define these before including the header to enable and disable sort types
//#define ENABLE_OPENCL_RADIXSORT
//#define ENABLE_DX11_RADIXSORT



// TODO: Most of these constants can die or better be consts in the appropriate subclass based on tuning.
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







#ifdef _WIN32
typedef unsigned __int32 uint32_t;
#endif // #ifdef _WIN32

typedef struct {
    unsigned int key;
    unsigned int value;
} KeyValuePair;



#include "radixsort_parent.hpp"

#if defined(ENABLE_OPENCL_RADIXSORT)
#include "radixsort_cl.hpp"
#endif // #if defined(USE_OPENCL)

#if defined(ENABLE_DX11_RADIXSORT)
#include "radixsort_cs5.hpp"
#endif // #if defined(USE_DX11)




#endif  // #ifndef RADIXSORT_HPP
