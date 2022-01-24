#pragma once

#include "CLContext.h"
#include "CLKernel.h"
#include <iostream>
#include <string>

class CLProgram
{
public:
    CLProgram();
    CLProgram(CLProgram&&);
    CLProgram(const CLProgram&);
   ~CLProgram();

    CLProgram& operator=(CLProgram&&);
    CLProgram& operator=(const CLProgram&);

    bool Create(cl_context, const std::string&, const std::string& options = std::string(), std::string& log = std::string());
    bool Create(cl_context, std::istream&, const std::string& options = std::string(), std::string& log = std::string());

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