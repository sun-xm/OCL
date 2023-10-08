#pragma once

#include "CLBuffer.h"
#include "CLLocal.h"
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class CLKernel
{
public:
    CLKernel() : kernel(nullptr)
    {
    }
    CLKernel(cl_kernel kernel) : CLKernel()
    {
        if (kernel && CL_SUCCESS == clRetainKernel(kernel))
        {
            this->kernel = kernel;
        }
    }
    CLKernel(CLKernel&& other) : CLKernel()
    {
        *this = std::move(other);
    }
    CLKernel(const CLKernel& other) : CLKernel()
    {
        *this = other;
    }
    virtual ~CLKernel()
    {
        if (this->kernel)
        {
            clReleaseKernel(this->kernel);
        }
    }

    CLKernel& operator=(CLKernel&& other)
    {
        cl_kernel kernel = this->kernel;
        this->kernel = other.kernel;
        other.kernel = kernel;

        this->event = std::move(other.event);

        this->global.swap(other.global);
        this->local.swap(other.local);

        return *this;
    }
    CLKernel& operator=(const CLKernel& other)
    {
        if (other.kernel && CL_SUCCESS != clRetainKernel(other.kernel))
        {
            throw std::runtime_error("Failed to retain kernel");
        }

        if (this->kernel)
        {
            clReleaseKernel(this->kernel);
        }

        this->kernel = other.kernel;
        return *this;
    }

    CLEvent& Event() const
    {
        return this->event;
    }
    operator cl_event() const
    {
        return (cl_event)this->event;
    }

    void Wait() const
    {
        this->event.Wait();
    }

    void Size(const std::vector<size_t>& global, const std::vector<size_t>& local = {})
    {
        this->global = global;
        this->local  = local;
    }

    const size_t* Global() const
    {
        return this->global.empty() ? nullptr : this->global.data();
    }

    const size_t* Local() const
    {
        return this->local.empty() ? nullptr : this->local.data();
    }

    cl_uint Dims() const
    {
        return (cl_uint)this->global.size();
    }

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

    static CLKernel Create(cl_program program, const std::string& name)
    {
        auto kernel = clCreateKernel(program, name.c_str(), nullptr);
        ONCLEANUP(kernel, [=]{ if(kernel) clReleaseKernel(kernel); });
        return CLKernel(kernel);
    }

protected:
    bool SetArgs(cl_uint index, const CLLocal& local)
    {
        auto err = clSetKernelArg(this->kernel, index, local.Size, nullptr);
    }

    template<typename T>
    bool SetArgs(cl_uint index, const CLBuffer<T>& buffer)
    {
        auto mem = (cl_mem)buffer;
        auto err = clSetKernelArg(this->kernel, index, sizeof(mem), &mem);
        return CL_SUCCESS == err;
    }

    template<typename T, typename... Tx>
    bool SetArgs(cl_uint index, const CLBuffer<T>& buffer, const Tx&... args)
    {
        auto mem = (cl_mem)buffer;
        auto err = clSetKernelArg(this->kernel, index, sizeof(mem), &mem);
        if (CL_SUCCESS != err)
        {
            return false;
        }

        return this->SetArgs(index + 1, args...);
    }

    template<typename T0>
    bool SetArgs(cl_uint index, const T0& arg0)
    {
        auto err = clSetKernelArg(this->kernel, index, sizeof(arg0), &arg0);
        return CL_SUCCESS == err;
    }

    template<typename T0, typename... Tx>
    bool SetArgs(cl_uint index, const T0& arg0, const Tx&... args)
    {
        auto err = clSetKernelArg(this->kernel, index, sizeof(arg0), &arg0);
        if (CL_SUCCESS != err)
        {
            return false;
        }

        return this->SetArgs(index + 1, args...);
    }

protected:
    cl_kernel kernel;
    std::vector<size_t> global;
    std::vector<size_t> local;

    mutable CLEvent event;
};