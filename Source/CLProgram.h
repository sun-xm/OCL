#pragma once

#include "CLKernel.h"
#include <iostream>
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

    bool GetBinary(std::vector<std::vector<uint8_t>>& binaries)
    {
        if (!this->program)
        {
            return false;
        }

        cl_uint devices;
        cl_int error = clGetProgramInfo(this->program, CL_PROGRAM_NUM_DEVICES, sizeof(devices), &devices, nullptr);
        if (CL_SUCCESS != error)
        {
            return false;
        }

        std::vector<size_t> sizes(devices);
        error = clGetProgramInfo(this->program, CL_PROGRAM_BINARY_SIZES, sizes.size() * sizeof(sizes[0]), &sizes[0], nullptr);
        if (CL_SUCCESS != error)
        {
            return false;
        }

        binaries.resize(devices);
        std::vector<uint8_t*> pointers;

        for (cl_uint i = 0; i < devices; i++)
        {
            auto& binary = binaries[i];
            binary.resize(sizes[i]);
            pointers.push_back(&binary[0]);
        }

        error = clGetProgramInfo(this->program, CL_PROGRAM_BINARIES, pointers.size() * sizeof(pointers[0]), &pointers[0], nullptr);
        if (CL_SUCCESS != error)
        {
            return false;
        }

        return true;
    }

    bool Save(std::ostream& stream)
    {
        std::vector<std::vector<uint8_t>> binaries;
        if (!this->GetBinary(binaries))
        {
            return false;
        }

        for (auto& binary : binaries)
        {
            int size = (int)binary.size();
            stream.write((char*)&size, sizeof(size));
            stream.write((char*)binary.data(), size);
        }

        return !!stream;
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