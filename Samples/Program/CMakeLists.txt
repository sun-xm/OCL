cmake_minimum_required(VERSION 3.7)
project(Program)
add_executable(${PROJECT_NAME}
               ${SOURCE_CPP}
               main.cpp)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/Program.cl" "${CMAKE_CURRENT_LIST_DIR}/../../build")