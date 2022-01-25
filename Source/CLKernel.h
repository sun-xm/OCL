#pragma once

#include "CLBuffer.h"
#include <initializer_list>
#include <exception>
#include <string>
#include <vector>

class CLKernel
{
private:
    CLKernel(cl_kernel);
public:
    CLKernel(CLKernel&&);
    CLKernel(const CLKernel&);
   ~CLKernel();

    CLKernel& operator=(CLKernel&&);
    CLKernel& operator=(const CLKernel&);

    CLEvent& Event() const
    {
        return this->event;
    }
    operator CLEvent&() const
    {
        return this->event;
    }
    
    void Size(const std::initializer_list<size_t>& global, const std::initializer_list<size_t>& local = {});

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

    static CLKernel Create(cl_program, const std::string&);

private:
    bool SetArgs(cl_uint index, const CLBuffer& buffer)
    {
        auto mem = (cl_mem)buffer;
        return CL_SUCCESS == clSetKernelArg(this->kernel, index, sizeof(mem), &mem);
    }

    template<typename T0>
    bool SetArgs(cl_uint index, const T0& arg0)
    {
        return CL_SUCCESS == clSetKernelArg(this->kernel, index, sizeof(arg0), &arg0);
    }

    template<typename... Tx>
    bool SetArgs(cl_uint index, const CLBuffer& buffer, const Tx&... args)
    {
        auto mem = (cl_mem)buffer;
        if (CL_SUCCESS != clSetKernelArg(this->kernel, index, sizeof(mem), &mem))
        {
            return false;
        }

        return this->SetArgs(index + 1, args...);
    }

    template<typename T0, typename... Tx>
    bool SetArgs(cl_uint index, const T0& arg0, const Tx&... args)
    {
        if (CL_SUCCESS != clSetKernelArg(this->kernel, index, sizeof(arg0), &arg0))
        {
            return false;
        }

        return this->SetArgs(index + 1, args...);
    }

private:
    cl_kernel kernel;
    std::vector<size_t> global;
    std::vector<size_t> local;

    mutable CLEvent event;
};