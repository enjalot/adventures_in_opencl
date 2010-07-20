CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(part1)

#need to include FindOPENCL.cmake to
SET( CMAKE_MODULE_PATH ${part1_SOURCE_DIR}/../cmake)

message("CMake module path: ${CMAKE_MODULE_PATH}\n")

FIND_PACKAGE (OPENCL)

INCLUDE_DIRECTORIES( 
    ${part1_SOURCE_DIR}/../opencl10
)

message("include directories: \n")
get_directory_property(includes INCLUDE_DIRECTORIES)
message("${includes}\n")

#provide the source path so we can open our .cl file at runtime
#ADD_DEFINITIONS(-DCL_SOURCE_DIR="${part1_SOURCE_DIR}")
#build in debug mode
ADD_DEFINITIONS(-g )

#the source files we want to compile into the library
set (LIBCXXFILES cll.cpp part1.cpp util.cpp)
#set a CMake variable to name the library
SET(cllib part1)
ADD_LIBRARY(${cllib} ${LIBCXXFILES})


#create the executable
SET(EXEC part1.x)
ADD_EXECUTABLE(${EXEC} main.cpp)

TARGET_LINK_LIBRARIES (${EXEC}
   ${cllib}
   ${OPENCL_LIBRARIES}
)
