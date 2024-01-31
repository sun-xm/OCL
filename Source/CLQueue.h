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

    template<typename T>
    CLMemMap<T> Map(CLBuffer<T>& buffer, uint32_t flags)
    {
        return buffer.Map(this->queue, flags);
    }
    template<typename T>
    CLMemMap<T> Map(CLBuffer<T>& buffer, uint32_t flags, size_t offset, size_t length)
    {
        return buffer.Map(this->queue, flags, offset, length);
    }
    template<typename T>
    CLMemMap<T> Map(CLBuffer<T>& buffer, uint32_t flags, const std::vector<cl_event>& waits)
    {
        return buffer.Map(this->queue, flags, waits);
    }
    template<typename T>
    CLMemMap<T> Map(CLBuffer<T>& buffer, uint32_t flags, const std::vector<cl_event>& waits, size_t offset, size_t length)
    {
        return buffer.Map(this->queue, flags, waits, offset, length);
    }

    template<typename T>
    bool Copy(const CLBuffer<T>& src, CLBuffer<T>& dst)
    {
        return dst.Copy(this->queue, src);
    }
    template<typename T>
    bool Copy(const CLBuffer<T>& src, CLBuffer<T>& dst, size_t srcoff, size_t dstoff, size_t length)
    {
        return dst.Copy(this->queue, src, srcoff, dstoff, length);
    }
    template<typename T>
    bool Copy(const CLBuffer<T>& src, CLBuffer<T>& dst, const std::vector<cl_event>& waits)
    {
        return dst.Copy(this->queue, src, waits);
    }
    template<typename T>
    bool Copy(const CLBuffer<T>& src, CLBuffer<T>& dst, size_t srcoff, size_t dstoff, size_t length, const std::vector<cl_event>& waits)
    {
        return dst.Copy(this->queue, src, srcoff, dstoff, length, waits);
    }

    template<typename T>
    bool Read(const CLBuffer<T>& buffer, T* host)
    {
        return buffer.Read(this->queue, host);
    }
    template<typename T>
    bool Read(const CLBuffer<T>& buffer, size_t offset, size_t length, T* host)
    {
        return buffer.Read(this->queue, offset, length, host);
    }
    template<typename T>
    bool Read(const CLBuffer<T>& buffer, T* host, const std::vector<cl_event>& waits)
    {
        return buffer.Read(this->queue, host, waits);
    }
    template<typename T>
    bool Read(const CLBuffer<T>& buffer, size_t offset, size_t length, T* host, const std::vector<cl_event&>& waits)
    {
        return buffer.Read(this->queue, offset, length, host, waits);
    }

    template<typename T>
    bool Write(CLBuffer<T>& buffer, const T* host)
    {
        return buffer.Write(this->queue, host);
    }
    template<typename T>
    bool Write(CLBuffer<T>& buffer, size_t offset, size_t length, const T* host)
    {
        return buffer.Write(this->queue, offset, length, host);
    }
    template<typename T>
    bool Write(CLBuffer<T>& buffer, const T* host, const std::vector<cl_event>& waits)
    {
        return buffer.Write(this->queue, host, waits);
    }
    template<typename T>
    bool Write(CLBuffer<T>& buffer, size_t offset, size_t length, const T* host, const std::vector<cl_event>& waits)
    {
        return buffer.Write(this->queue, offset, length, host, waits);
    }

    bool Execute(const CLKernel& kernel)
    {
        if (!this->Execute(kernel, {}))
        {
            return false;
        }

        kernel.Wait();
        return true;
    }
    bool Execute(const CLKernel& kernel, const std::vector<cl_event>& waits)
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
        auto err = clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != err)
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