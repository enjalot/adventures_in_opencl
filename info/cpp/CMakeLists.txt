CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(info)

#need to include FindOPENCL.cmake to
SET( CMAKE_MODULE_PATH ${info_SOURCE_DIR}/../../cmake)

message("CMake module path: ${CMAKE_MODULE_PATH}\n")

FIND_PACKAGE (OPENCL)

INCLUDE_DIRECTORIES( 
    ${info_SOURCE_DIR}/../../opencl10
)

message("include directories: \n")
get_directory_property(includes INCLUDE_DIRECTORIES)
message("${includes}\n")

#provide the source path so we can open our .cl file at runtime
#ADD_DEFINITIONS(-DCL_SOURCE_DIR="${info_SOURCE_DIR}")
#build in debug mode
ADD_DEFINITIONS(-g )

#the source files we want to compile into the library
set (LIBCXXFILES main.cpp util.cpp)
#set a CMake variable to name the library
SET(cllib info)
ADD_LIBRARY(${cllib} ${LIBCXXFILES})


#create the executable
SET(EXEC info.x)
ADD_EXECUTABLE(${EXEC} main.cpp)

TARGET_LINK_LIBRARIES (${EXEC}
   ${cllib}
   ${OPENCL_LIBRARIES}
)
