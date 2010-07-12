FIND_LIBRARY (accelerate NAMES Accelerate)
# CLAPACK is available from netlib.org or linux repositories
# Use Accelerate library on OSX
#FIND_LIBRARY (clapack clapack PATHS /usr/lib) 


#MESSAGE(STATUS "MacOS X detected. Added '-framework Accelerate' to compiler flags")

# One or more dirs split by spaces. This is a command so it can be called multiple times
INCLUDE_DIRECTORIES (AFTER
)

# One or more dirs split by spaces. This is a command so it can be called multiple times
LINK_DIRECTORIES (
)

#FIND_LIBRARY(swan_ocl swan_ocl PATHS /Users/erlebach/Documents/src/fftw-3.2.2/.libs)
#FIND_LIBRARY(swan_cuda swan_cuda PATHS ${SWAN}/lib)
	
# Additional libraries required by this OS
# NOTE: order of libraries is important in Linux. 
# 	does not matter on macOSX
SET (ADDITIONAL_REQUIRED_LIBRARIES 
)

#===================================================
FIND_PACKAGE(Cuda)
#CUDA_ADD_CUBLAS_TO_TARGET(target)
#CUDA_ADD_EXECUTABLE(target file1 ...)
#CUDA_ADD_LIBRARY(target file1 ...)
#CUDA_BUILD_CLEAN_TARGET()
#CUDA_INCLUDE_DIRECTORIES() # for nvcc
set (CUDA_SDK_ROOT_DIR $ENV{CUDA_LOCAL})  # set environment variable
message("${CUDA_TOOLKIT_ROOT_DIR}")
set(CCC $ENV{CUDA_LOCAL})   # environment variable
message("${CUDA_INCLUDE_DIRS}")
message("${CUDA_VERSION_MAJOR}")
message("${CUDA_VERSION_MINOR}")
set(CUDA_64_BIT_DEVICE_CODE off)
#set(CUDA_PROPAGATE_HOST_FLAGS off)


# define environment variables
message(sdk:   "$ENV{CUDA_LOCAL}")
message(sdk:   "${CUDA_LOCAL}")
message("${CCC}")
FIND_PACKAGE(Cuda)
#CUDA_ADD_CUBLAS_TO_TARGET(target)
#CUDA_ADD_EXECUTABLE(target file1 ...)
#CUDA_ADD_LIBRARY(target file1 ...)
#CUDA_BUILD_CLEAN_TARGET()
#CUDA_INCLUDE_DIRECTORIES() # for nvcc
set (CUDA_SDK_ROOT_DIR $ENV{CUDA_LOCAL})  # set environment variable
message("${CUDA_TOOLKIT_ROOT_DIR}")
set(CCC $ENV{CUDA_LOCAL})   # environment variable
message("${CUDA_INCLUDE_DIRS}")
message("${CUDA_VERSION_MAJOR}")
message("${CUDA_VERSION_MINOR}")
set(CUDA_64_BIT_DEVICE_CODE off)
#set(CUDA_NVCC_FLAGS -m32)
#set(CUDA_PROPAGATE_HOST_FLAGS off)


# define environment variables
message(sdk:   "$ENV{CUDA_LOCAL}")
message(sdk:   "${CUDA_LOCAL}")
message("${CCC}")

set(CUDA_NVCC_FLAGS -Xcompiler -m32)


#---------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)  
   set(cuda_cutil_name cutil_x86_64)
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
   set(cuda_cutil_name cutil32)
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

set(cuda_cutil_name cutil_i386)

message("cutil name: ${cuda_cutil_name}")

find_library(CUDA_CUT_LIBRARY
   NAMES cutil ${cuda_cutil_name} 
   PATHS ${CUDA_SDK_ROOT_DIR}
   # The new version of the sdk shows up in common/lib, but the old one is in lib
   PATH_SUFFIXES "common/lib" "lib"
   DOC "Location of cutil library"
   NO_DEFAULT_PATH
 )
# Now search system paths
find_library(CUDA_CUT_LIBRARY NAMES cutil ${cuda_cutil_name} DOC "Location of cutil library")
 mark_as_advanced(CUDA_CUT_LIBRARY)
 set(CUDA_CUT_LIBRARIES ${CUDA_CUT_LIBRARY})
#---------------------





# Mac is 64 bit, but must be compiled in 32 bit

# for mac (gfortran)
ADD_DEFINITIONS(-m32)

#===================================================
