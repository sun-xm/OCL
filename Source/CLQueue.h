#pragma once

#include "CLKernel.h"

class CLQueue
{
public:
    CLQueue() : queue(nullptr)
    {
    }
    CLQueue(cl_command_queue queue) : CLQueue()
    {
        if (queue && CL_SUCCESS == clRetainCommandQueue(queue))
        {
            this->queue = queue;
        }
    }
    CLQueue(CLQueue&& other) : CLQueue()
    {
        *this = std::move(other);
    }
    CLQueue(const CLQueue& other)
    {
        *this = other;
    }
    virtual ~CLQueue()
    {
        if (this->queue)
        {
            clReleaseCommandQueue(this->queue);
        }
    }

    CLQueue& operator=(CLQueue&& other)
    {
        auto queue = this->queue;
        this->queue = other.queue;
        other.queue = queue;
        return *this;
    }
    CLQueue& operator=(const CLQueue& other)
    {
        if (other.queue && CL_SUCCESS != clRetainCommandQueue(other.queue))
        {
            throw std::runtime_error("Failed to retain queue");
        }

        if (this->queue)
        {
            clReleaseCommandQueue(this->queue);
        }

        this->queue = other.queue;
        return *this;
    }

    void Finish()
    {
        clFinish(this->queue);
    }

    operator cl_command_queue() const
    {
        return this->queue;
    }

    static CLQueue Create(cl_context context, cl_device_id device = nullptr)
    {
        if (!device)
        {
            cl_uint num;
            cl_int error = clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(num), &num, nullptr);
            if (CL_SUCCESS != error || !num)
            {
                return CLQueue(nullptr);
            }

            error = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(device), &device, nullptr);
            if (CL_SUCCESS != error)
            {
                return CLQueue(nullptr);
            }
        }

        cl_int err;
#if CL_TARGET_OPENCL_VERSION >= 200
        auto queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
#else
        auto queue = clCreateCommandQueue(context, device, 0, &err);
#endif
        ONCLEANUP(queue, [=]{ if (queue) clReleaseCommandQueue(queue); });
        return CLQueue(queue);
    }

protected:
    cl_command_queue queue;
};