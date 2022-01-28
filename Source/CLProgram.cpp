#include "CLProgram.h"
#include <utility>

using namespace std;

CLProgram::CLProgram() : program(nullptr)
{
}

CLProgram::CLProgram(cl_program program) : CLProgram()
{
    if (program && CL_SUCCESS == clRetainProgram(program))
    {
        this->program = program;
    }
}

CLProgram::CLProgram(CLProgram&& other)
{
    *this = move(other);
}

CLProgram::CLProgram(const CLProgram& other) : CLProgram()
{
    *this = other;
}

CLProgram::~CLProgram()
{
    if (this->program)
    {
        clReleaseProgram(this->program);
    }
}

CLProgram& CLProgram::operator=(CLProgram&& other)
{
    if (this->program)
    {
        clReleaseProgram(this->program);
    }

    this->program = other.program;
    other.program = nullptr;
    return *this;
}

CLProgram& CLProgram::operator=(const CLProgram& other)
{
    if (other.program && CL_SUCCESS != clRetainProgram(other.program))
    {
        throw runtime_error("Failed to retain program");
    }

    if (this->program)
    {
        clReleaseProgram(this->program);
    }

    this->program = other.program;
    return *this;
}

CLKernel CLProgram::CreateKernel(const string& name)
{
    return CLKernel::Create(this->program, name);
}