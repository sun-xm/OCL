#pragma once

#include "CLCommon.h"
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
    virtual ~CLProgram()
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

    static CLProgram Create(cl_context context, const char* source, const char* options, std::string& log)
    {
        size_t size;
        cl_int error = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, nullptr, &size);
        if (CL_SUCCESS != error)
        {
            log = "Failed to get context devices number";
            return CLProgram();
        }

        std::vector<cl_device_id> devices(size / sizeof(cl_device_id));
        if (devices.empty())
        {
            log = "No associated devices to context";
            return CLProgram();
        }

        error = clGetContextInfo(context, CL_CONTEXT_DEVICES, size, &devices[0], nullptr);
        if (CL_SUCCESS != error)
        {
            log = "Failed to get context devices";
            return CLProgram();
        }

        auto length = strlen(source);
        auto program = clCreateProgramWithSource(context, 1, &source, &length, &error);
        if (CL_SUCCESS != error)
        {
            log = "Failed to create program";
            return CLProgram();
        }
        ONCLEANUP(program, [=]{ clReleaseProgram(program); });

        error = clBuildProgram(program, (cl_uint)devices.size(), devices.data(), options, nullptr, nullptr);
        if (CL_SUCCESS != error)
        {
            log = "Failed to build program\n";

            for (auto device : devices)
            {
                cl_build_status status;
                error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
                if (CL_SUCCESS != error)
                {
                    log += "Failed to get program build status";
                    break;
                }

                if (CL_BUILD_ERROR == status)
                {
                    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size);
                    if (CL_SUCCESS != error)
                    {
                        log += "Failed to get program build log length";
                        break;
                    }

                    std::string err;
                    err.resize(size);

                    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size, &err[0], nullptr);
                    if (CL_SUCCESS != error)
                    {
                        log += "Failed to get program build log";
                        break;
                    }

                    err.resize(err.size() - 1);
                    log += err;
                    break;
                }
            }

            return CLProgram();
        }

        return CLProgram(program);
    }
    static CLProgram Create(cl_context context, std::istream& source, const char* options, std::string& log)
    {
        return Create(context, std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()).c_str(), options, log);
    }

    static CLProgram Load(cl_context context, const std::vector<std::vector<uint8_t>>& binaries, std::string& log, std::vector<cl_int>* status = nullptr)
    {
        if (binaries.empty())
        {
            return CLProgram();
        }

        size_t size;
        cl_int error = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, nullptr, &size);
        if (CL_SUCCESS != error)
        {
            log = "Failed to get context devices number";
            return CLProgram();
        }

        std::vector<cl_device_id> devices(size / sizeof(cl_device_id));
        if (devices.empty())
        {
            log = "No associated devices to context";
            return CLProgram();
        }

        error = clGetContextInfo(context, CL_CONTEXT_DEVICES, size, &devices[0], nullptr);
        if (CL_SUCCESS != error)
        {
            log = "Failed to get context devices";
            return CLProgram();
        }

        std::vector<size_t> lengths;
        std::vector<const uint8_t*> pointers;
        for (auto& binary : binaries)
        {
            lengths.push_back(binary.size());
            pointers.push_back(binary.data());
        }

        if (status)
        {
            status->resize(binaries.size());
        }

        auto program = clCreateProgramWithBinary(context, (cl_uint)devices.size(), devices.data(), lengths.data(), pointers.data(), status ? &(*status)[0] : nullptr, &error);

        if (CL_SUCCESS != error)
        {
            log = "Failed to create program";
            return CLProgram();
        }
        ONCLEANUP(program, [=]{ clReleaseProgram(program); });

        error = clBuildProgram(program, (cl_uint)devices.size(), devices.data(), nullptr, nullptr, nullptr);
        if (CL_SUCCESS != error)
        {
            log = "Failed to build program\n";

            for (auto device : devices)
            {
                cl_build_status status;
                error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr);
                if (CL_SUCCESS != error)
                {
                    log += "Failed to get program build status";
                    break;
                }

                if (CL_BUILD_ERROR == status)
                {
                    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size);
                    if (CL_SUCCESS != error)
                    {
                        log += "Failed to get program build log length";
                        break;
                    }

                    std::string err;
                    err.resize(size);

                    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size, &err[0], nullptr);
                    if (CL_SUCCESS != error)
                    {
                        log += "Failed to get program build log";
                        break;
                    }

                    err.resize(err.size() - 1);
                    log += err;
                    break;
                }
            }

            return CLProgram();
        }

        return CLProgram(program);
    }
    static CLProgram Load(cl_context context, std::istream& stream, std::string& log, std::vector<cl_int>* status = nullptr)
    {
        std::vector<std::vector<uint8_t>> binaries;

        while (true)
        {
            int size;
            stream.read((char*)&size, sizeof(size));
            if (!stream)
            {
                return Load(context, binaries, log, status);
            }

            std::vector<uint8_t> binary(size);
            stream.read((char*)&binary[0], binary.size());
            if (!stream)
            {
                return false;
            }
            binaries.push_back(std::move(binary));
        }
    }

protected:
    cl_program program;
};