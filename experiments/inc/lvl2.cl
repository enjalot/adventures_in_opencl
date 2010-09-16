#ifndef LVL_2_CLH
#define LVL_2_CLH

#ifdef POSITIVE
//#ifdef NEGATIVE
    __constant float def_sign = 1.0f;
#else
    #ifdef NEGATIVE
    //#ifdef POSITIVE
        __constant float def_sign = -1.0f;
    #endif
#endif

#endif
