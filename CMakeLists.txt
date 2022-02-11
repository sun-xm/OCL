cmake_minimum_required(VERSION 3.10)
project(OCL)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/Source)

include_directories($ENV{OPENCL_PATH}/include ${SOURCE_DIR})

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    link_directories($ENV{OPENCL_PATH}/lib/$(PlatformShortName))
    add_link_options(/DYNAMICBASE OpenCL.lib)
elseif(CMAKE_GENERATOR MATCHES "MinGW")
    find_library(OPENCL_LIB NAMES OpenCL PATHS $ENV{OPENCL_PATH}/lib/x64 REQUIRED)
else()
    message(FATAL_ERROR "Unknown generator")
endif()

add_compile_definitions(CL_TARGET_OPENCL_VERSION=220)

enable_testing()
add_subdirectory(Test Test)