cmake_minimum_required(VERSION 3.10)
project(OCL)

set(CMAKE_CXX_STANDARD 11)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/Source)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIB_DIR "$ENV{OPENCL_PATH}/lib/x64")
else()
    set(LIB_DIR "$ENV{OPENCL_PATH}/lib/x86"
                "$ENV{OPENCL_PATH}/lib/win32")
endif()

include_directories($ENV{OPENCL_PATH}/include ${SRC_DIR})
find_library(OPENCL NAMES OpenCL PATHS ${LIB_DIR} REQUIRED)

add_compile_definitions(CL_TARGET_OPENCL_VERSION=220)

enable_testing()
add_subdirectory(Test Test)