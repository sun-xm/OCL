#pragma once

#include "CLKernel.h"
#include <string>

class CLProgram
{
public:
    CLProgram();
    CLProgram(cl_program);
    CLProgram(CLProgram&&);
    CLProgram(const CLProgram&);
   ~CLProgram();

    CLProgram& operator=(CLProgram&&);
    CLProgram& operator=(const CLProgram&);

    CLKernel CreateKernel(const std::string& name);

    operator cl_program() const
    {
        return this->program;
    }

    operator bool() const
    {
        return !!this->program;
    }
    
private:
    cl_program program;
};