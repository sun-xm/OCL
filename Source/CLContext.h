#pragma once

#include "CLCommon.h"
#include "CLPlatform.h"
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