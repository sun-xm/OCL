cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/Source)

include_directories($ENV{OPENCL_PATH}/include ${SOURCE_DIR})
link_directories($ENV{OPENCL_PATH}/lib/$(PlatformShortName))

add_compile_definitions(CL_TARGET_OPENCL_VERSION=220)

enable_testing()
add_subdirectory(Test Test)