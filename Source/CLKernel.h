#pragma once

#include <exception>
#include <CL/cl.h>

class CLKernel
{
public:
    CLKernel(cl_kernel);
    CLKernel(CLKernel&&);
    CLKernel(const CLKernel&) = delete;
   ~CLKernel();

    CLKernel& operator=(CLKernel&& other)
    {
        if (this->kernel)
        {
            clReleaseKernel(this->kernel);
        }

        this->kernel = other.kernel;
        other.kernel = nullptr;
        return *this;
    }
    CLKernel& operator=(const CLKernel&) = delete;

    template<typename T0, typename... Tx>
    bool Args(const T0& arg0, const Tx&... args)
    {
        return this->SetArgs(0, arg0, args...);
    }

    operator cl_kernel() const
    {
        return this->kernel;
    }

    operator bool() const
    {
        return !!this->kernel;
    }
private:
    template<typename T0>
    bool SetArgs(cl_uint index, const T0& arg0)
    {
        return CL_SUCCESS == clSetKernelArg(this->kernel, index, sizeof(arg0), &arg0);
    }

    template<typename T0, typename... Tx>
    bool SetArgs(cl_uint index, const T0& arg0, const Tx... args)
    {
        if (CL_SUCCESS != clSetKernelArg(this->kernel, index, sizeof(arg0), &arg0))
        {
            return false;
        }

        return this->SetArgs(index + 1, args...);
    }

private:
    cl_kernel kernel;
};