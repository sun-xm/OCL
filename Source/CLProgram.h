#pragma once

#include "CLKernel.h"
#include <string>

class CLProgram
{
public:
    CLProgram() : program(nullptr)
    {
    }
    CLProgram(cl_program program) : CLProgram()
    {
        if (program && CL_SUCCESS == clRetainProgram(program))
        {
            this->program = program;
        }
    }
    CLProgram(CLProgram&& other) : CLProgram()
    {
        *this = std::move(other);
    }
    CLProgram(const CLProgram& other) : CLProgram()
    {
        *this = other;
    }
   ~CLProgram()
    {
        if (this->program)
        {
            clReleaseProgram(this->program);
        }
    }

    CLProgram& operator=(CLProgram&& other)
    {
        cl_program program = this->program;
        this->program = other.program;
        other.program = program;
        return *this;
    }
    CLProgram& operator=(const CLProgram& other)
    {
        if (other.program && CL_SUCCESS != clRetainProgram(other.program))
        {
            throw std::runtime_error("Failed to retain program");
        }

        if (this->program)
        {
            clReleaseProgram(this->program);
        }

        this->program = other.program;
        return *this;
    }

    CLKernel CreateKernel(const std::string& name)
    {
        return CLKernel::Create(this->program, name);
    }

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