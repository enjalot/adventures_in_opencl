#ifndef ADVCL_CLL_H_INCLUDED
#define ADVCL_CLL_H_INCLUDED

#if defined __APPLE__ || defined(MACOSX)
    #include <OpenCL/opencl.h>
#else
    #include <CL/opencl.h>
#endif

class CL {
    public:
        cl_context context;

        CL();
        ~CL();

    private:
        cl_event event;
};

#endif
