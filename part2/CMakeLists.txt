CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(part2)

#need to include FindOPENCL.cmake to
SET( CMAKE_MODULE_PATH ${part2_SOURCE_DIR}/../cmake)

message("CMake module path: ${CMAKE_MODULE_PATH}\n")

FIND_PACKAGE (OPENCL)
#Need some OpenGL packages
FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLEW)
FIND_PACKAGE(GLUT)

INCLUDE_DIRECTORIES( 
    ${GLUT_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_PATH}
    ${part2_SOURCE_DIR}/../opencl10
)

message("include directories: \n")
get_directory_property(includes INCLUDE_DIRECTORIES)
message("${includes}\n")

#build in debug mode
ADD_DEFINITIONS(-g)

#we want OpenCL to share context with OpenGL
#this isn't available on all cards so we should provide
#alternative option in the code
ADD_DEFINITIONS(-DGL_INTEROP)

#the source files we want to compile into the library
set (LIBCXXFILES cll.cpp part2.cpp util.cpp)
#set a CMake variable to name the library
SET(cllib part2)
ADD_LIBRARY(${cllib} ${LIBCXXFILES})


#create the executable
SET(EXEC part2.x)
ADD_EXECUTABLE(${EXEC} main.cpp)

TARGET_LINK_LIBRARIES (${EXEC}
   ${cllib}
   ${GLUT_LIBRARIES}
   ${OPENGL_LIBRARIES}
   ${GLEW_LIBRARY}
   ${OPENCL_LIBRARIES}
)
