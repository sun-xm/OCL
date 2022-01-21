set(CMAKE_CXX_STANDARD 11)
set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/Source)
set(SOURCE_CPP ${SOURCE_DIR}/CLPlatform.cpp
               ${SOURCE_DIR}/CLDevice.cpp
               ${SOURCE_DIR}/CLContext.cpp
               ${SOURCE_DIR}/CLProgram.cpp
               ${SOURCE_DIR}/CLKernel.cpp
               ${SOURCE_DIR}/CLQueue.cpp)

include_directories($ENV{OPENCL_PATH}include ${SOURCE_DIR})
link_directories($ENV{OPENCL_PATH}lib/$(PlatformShortName))

add_compile_definitions(CL_TARGET_OPENCL_VERSION=220)

add_subdirectory(Samples/Device   Device)
add_subdirectory(Samples/Platform Platform)
add_subdirectory(Samples/Program  Program)