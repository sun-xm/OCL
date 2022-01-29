#pragma once

#include "CLCommon.h"
#include "CLMemMap.h"
#include <cstdint>
#include <stdexcept>
#include <utility>

template<typename T>
class CLBuffer
{
public:
    CLBuffer() : mem(nullptr)
    {
    }
    CLBuffer(cl_mem mem) : CLBuffer()
    {
        if (mem && CL_SUCCESS == clRetainMemObject(mem))
        {
            this->mem = mem;
        }
    }
    CLBuffer(CLBuffer&& other) : CLBuffer()
    {
        *this = std::move(other);
    }
    CLBuffer(const CLBuffer& other) : CLBuffer()
    {
        *this = other;
    }
   ~CLBuffer()
    {
        if (this->mem)
        {
            clReleaseMemObject(this->mem);
        }
    }

    CLBuffer<T>& operator=(CLBuffer&& other)
    {
        cl_mem mem = other.mem;
        this->mem = other.mem;
        other.mem = mem;
        return *this;
    }
    CLBuffer<T>& operator=(const CLBuffer& other)
    {
        if (other.mem && CL_SUCCESS != clRetainMemObject(other.mem))
        {
            throw runtime_error("Failed to retain buffer");
        }

        if (this->mem)
        {
            clReleaseMemObject(this->mem);
        }

        this->mem = other.mem;
        return *this;
    }

    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Map(queue, flags, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, const std::initializer_list<CLEvent>& waits)
    {
        size_t bytes;
        if (CL_SUCCESS != clGetMemObjectInfo(this->mem, CL_MEM_SIZE,  sizeof(bytes), &bytes, nullptr))
        {
            return CLMemMap<T>(nullptr, nullptr, nullptr, nullptr);
        }

        cl_map_flags mflags = 0;
        if (CLFlags::RO & flags)
        {
            mflags |= CL_MAP_READ;
        }
        if (CLFlags::WO & flags)
        {
            mflags |= CL_MAP_WRITE;
        }
        
        vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        cl_int error;
        cl_event event;
        auto map = clEnqueueMapBuffer(queue, this->mem, CL_FALSE, mflags, 0, bytes, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event, &error);

        if (CL_SUCCESS != error)
        {
            return CLMemMap<T>(nullptr, nullptr, nullptr, nullptr);
        }

        this->event = CLEvent(event);
        clReleaseEvent(event);

        return CLMemMap<T>(this->mem, queue, event, map);
    }

    void Wait() const
    {
        this->event.Wait();
    }

    size_t Size() const
    {
        if (!this->mem)
        {
            return 0;
        }

        size_t size;
        if (CL_SUCCESS != clGetMemObjectInfo(this->mem, CL_MEM_SIZE, sizeof(size), &size, nullptr))
        {
            throw runtime_error("Faile dto get mem object size");
        }

        return size;
    }
    size_t Length() const
    {
        return this->Size() / sizeof(T);
    }

    CLEvent Event() const
    {
        return this->event;
    }
    operator CLEvent&() const
    {
        return this->event;
    }

    operator bool() const
    {
        return !!this->mem;
    }

    operator cl_mem() const
    {
        return this->mem;
    }
    
    static CLBuffer Create(cl_context context, uint32_t flags, size_t length)
    {
        cl_mem_flags mflags;
        switch (flags)
        {
            case CLFlags::RW:
            {
                mflags = CL_MEM_READ_WRITE;
                break;
            }

            case CLFlags::RO:
            {
                mflags = CL_MEM_READ_ONLY;
                break;
            }

            case CLFlags::WO:
            {
                mflags = CL_MEM_WRITE_ONLY;
                break;
            }

            default:
                throw runtime_error("Invalid memory creation flag");
        }

        auto buf = clCreateBuffer(context, mflags, length * sizeof(T), nullptr, nullptr);
        return CLBuffer(buf);
    }

private:

private:
    cl_mem mem;
    mutable CLEvent event;
};