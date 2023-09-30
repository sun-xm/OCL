#pragma once

#include "CLBuffer.h"
#include "CLDevice.h"
#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLQueue.h"
#include <iostream>
#include <string>
#include <vector>

class CLContext
{
public:
    CLContext() : context(nullptr)
    {
    }
    CLContext(cl_context context) : CLContext()
    {
        if (context && CL_SUCCESS == clRetainContext(context))
        {
            this->context = context;
        }
    }
    CLContext(CLContext&& other) : CLContext()
    {
        *this = std::move(other);
    }
    CLContext(const CLContext& other) : CLContext()
    {
        *this = other;
    }
    virtual ~CLContext()
    {
        if (this->context)
        {
            clReleaseContext(this->context);
        }
    }

    CLContext& operator=(CLContext&& other)
    {
        cl_context context = this->context;
        this->context = other.context;
        other.context = context;
        return *this;
    }
    CLContext& operator=(const CLContext& other)
    {
        if (other.context && CL_SUCCESS != clRetainContext(other.context))
        {
            throw std::runtime_error("Failed to retain context");
        }

        if (this->context)
        {
            clReleaseContext(this->context);
        }

        this->context = other.context;
        return *this;
    }

    CLProgram LoadProgram(const std::vector<std::vector<uint8_t>>& binaries, std::string& log, std::vector<cl_int>* status = nullptr)
    {
        if (binaries.empty())
        {
            return CLProgram();
        }

        size_t size;
        cl_int error = clGetContextInfo(this->context, CL_CONTEXT_DEVICES, 0, nullptr, &size);
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

        error = clGetContextInfo(this->context, CL_CONTEXT_DEVICES, size, &devices[0], nullptr);
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

        auto program = clCreateProgramWithBinary(this->context, (cl_uint)devices.size(), devices.data(), lengths.data(), pointers.data(), status ? &(*status)[0] : nullptr, &error);

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
    CLProgram LoadProgram(std::istream& stream, std::string& log, std::vector<cl_int>* status = nullptr)
    {
        std::vector<std::vector<uint8_t>> binaries;

        while (true)
        {
            int size;
            stream.read((char*)&size, sizeof(size));
            if (!stream)
            {
                return this->LoadProgram(binaries, log, status);
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

    CLProgram CreateProgram(const char* source, const char* options, std::string& log)
    {
        size_t size;
        cl_int error = clGetContextInfo(this->context, CL_CONTEXT_DEVICES, 0, nullptr, &size);
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

        error = clGetContextInfo(this->context, CL_CONTEXT_DEVICES, size, &devices[0], nullptr);
        if (CL_SUCCESS != error)
        {
            log = "Failed to get context devices";
            return CLProgram();
        }

        auto length = strlen(source);
        auto program = clCreateProgramWithSource(this->context, 1, &source, &length, &error);
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
    CLProgram CreateProgram(std::istream& source, const char* options, std::string& log)
    {
        return this->CreateProgram(std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()).c_str(), options, log);
    }
    CLQueue   CreateQueue()
    {
        return CLQueue::Create(this->context);
    }

    template<typename T>
    CLBuffer<T> CreateBuffer(uint32_t flags, size_t length)
    {
        return CLBuffer<T>::Create(this->context, flags, length);
    }

    CLDevice Device() const
    {
        if (!this->context)
        {
            return CLDevice(nullptr);
        }

        cl_uint num;
        cl_int error = clGetContextInfo(this->context, CL_CONTEXT_NUM_DEVICES, sizeof(num), &num, nullptr);
        if (CL_SUCCESS != error || !num)
        {
            return CLDevice(nullptr);
        }

        std::vector<cl_device_id> ids(num);
        error = clGetContextInfo(this->context, CL_CONTEXT_DEVICES, ids.size() * sizeof(ids[0]), &ids[0], nullptr);
        if (CL_SUCCESS != error)
        {
            return CLDevice(nullptr);
        }

        return CLDevice(ids[0]);
    }

    operator cl_context() const
    {
        return this->context;
    }

    operator bool() const
    {
        return !!this->context;
    }

    static CLContext Create(cl_device_id device)
    {
        cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
        ONCLEANUP(context, [=]{ if (context) clReleaseContext(context); });
        return CLContext(context);
    }
    static CLContext CreateDefault()
    {
        auto platforms = CLPlatform::Platforms();
        if (platforms.empty())
        {
            return CLContext();
        }

        auto devices = platforms[0].Devices();
        if (devices.empty())
        {
            return CLContext();
        }

        return Create(devices[0]);
    }

protected:
    cl_context context;
};