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
   ~CLQueue()
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

    template<typename T>
    CLMemMap<T> Map(CLBuffer<T>& buffer, uint32_t flags)
    {
        return buffer.Map(this->queue, flags);
    }
    template<typename T>
    CLMemMap<T> Map(CLBuffer<T>& buffer, uint32_t flags, const std::initializer_list<CLEvent>& waits)
    {
        return buffer.Map(this->queue, flags, waits);
    }

    bool Execute(const CLKernel& kernel)
    {
        cl_event event;
        auto error = clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), 0, nullptr, &event);
        if (CL_SUCCESS != error)
        {
            return false;
        }
        
        kernel.Event() = CLEvent(event);
        clReleaseEvent(event);
        kernel.Event().Wait();
        return true;
    }
    bool Execute(const CLKernel& kernel, const std::initializer_list<CLEvent>& waits)
    {
        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        cl_event event;
        if (CL_SUCCESS != clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
        {
            return false;
        }

        kernel.Event() = CLEvent(event);
        clReleaseEvent(event);
        return true;
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
            if (CL_SUCCESS != clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(num), &num, nullptr) || !num)
            {
                return CLQueue(nullptr);
            }

            if (CL_SUCCESS != clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(device), &device, nullptr))
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

private:
    cl_command_queue queue;
};