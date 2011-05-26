//from http://www.songho.ca/opengl/gl_vbo.html

#ifndef ADVCL_UTIL_H_INCLUDED
#define ADVCL_UTIL_H_INCLUDED

char *file_contents(const char *filename, int *length);

cl_int oclGetPlatformID(cl_platform_id* clSelectedPlatformID);
const char* oclErrorString(cl_int error);


#endif
