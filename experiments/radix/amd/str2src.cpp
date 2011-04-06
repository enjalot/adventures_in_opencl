#include <stdio.h>
int main()
{
    #include "radixsort_cl_kernels.hpp"
    printf("%s", clKernelsString);
    return 0;
}
