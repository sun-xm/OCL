#pragma once

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

    operator cl_kernel() const
    {
        return this->kernel;
    }

    operator bool() const
    {
        return !!this->kernel;
    }

private:
    cl_kernel kernel;
};