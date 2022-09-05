#pragma once

#include "CLCommon.h"
#include "CLFlags.h"
#include "CLMemMap.h"
#include <cstdint>
#include <stdexcept>
#include <utility>

template<typename T>
class CLBuffer
{
public:
    CLBuffer() : mem(nullptr), len(0)
    {
    }
    CLBuffer(cl_mem mem, size_t length) : CLBuffer()
    {
        if (mem && length && CL_SUCCESS == clRetainMemObject(mem))
        {
            this->mem = mem;
            this->len = length;
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
        cl_mem mem = this->mem;
        size_t len = this->len;

        this->mem = other.mem;
        this->len = other.len;

        other.mem = mem;
        other.len = len;

        return *this;
    }
    CLBuffer<T>& operator=(const CLBuffer& other) = delete;

    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Map(queue, flags, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, size_t offset, size_t length)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Map(queue, flags, offset, length, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, const std::vector<cl_event>& waits)
    {
        return this->Map(queue, flags, 0, this->len, waits);
    }
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, size_t offset, size_t length, const std::vector<cl_event>& waits)
    {
        if (!this->mem)
        {
            return CLMemMap<T>();
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

        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        cl_int error;
        cl_event event;
        auto map = clEnqueueMapBuffer(queue, this->mem, CL_FALSE, mflags, offset * sizeof(T), length * sizeof(T), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event, &error);

        if (CL_SUCCESS != error)
        {
            return CLMemMap<T>();
        }

        this->event = CLEvent(event);
        clReleaseEvent(event);

        return CLMemMap<T>(this->mem, queue, event, map);
    }

    bool Copy(cl_command_queue queue, const CLBuffer<T>& source)
    {
        return this->Copy(queue, source, 0, 0, this->len < source.len ? this->len : source.len);
    }
    bool Copy(cl_command_queue queue, const CLBuffer<T>& source, size_t srcoff, size_t dstoff, size_t length)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Copy(queue, source, srcoff, dstoff, length, {});
    }
    bool Copy(cl_command_queue queue, const CLBuffer<T>& source, const std::vector<cl_event>& waits)
    {
        return this->Copy(queue, source, 0, 0, this->len < source.len ? this->len : source.len, waits);
    }
    bool Copy(cl_command_queue queue, const CLBuffer<T>& source, size_t srcoff, size_t dstoff, size_t length, const std::vector<cl_event>& waits)
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
        if (CL_SUCCESS != clEnqueueCopyBuffer(queue, source, this->mem, srcoff * sizeof(T), dstoff * sizeof(T), length * sizeof(T), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
        {
            return false;
        }

        this->event = CLEvent(event);
        return true;
    }

    bool Read(cl_command_queue queue, T* host) const
    {
        return this->Read(queue, 0, this->len, host);
    }
    bool Read(cl_command_queue queue, size_t offset, size_t length, T* host) const
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Read(queue, offset, length, host, {});
    }
    bool Read(cl_command_queue queue, T* host, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, 0, this->len, host, waits);
    }
    bool Read(cl_command_queue queue, size_t offset, size_t length, T* host, const std::vector<cl_event>& waits) const
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
        if (CL_SUCCESS != clEnqueueReadBuffer(queue, this->mem, CL_FALSE, offset * sizeof(T), length * sizeof(T), host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
        {
            return false;
        }

        this->event = CLEvent(event);
        return true;
    }

    bool Write(cl_command_queue queue, const T* host)
    {
        return this->Write(queue, 0, this->len, host);
    }
    bool Write(cl_command_queue queue, size_t offset, size_t length, const T* host)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Write(queue, offset, length, host, {});
    }
    bool Write(cl_command_queue queue, const T* host, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, 0, this->len, host, waits);
    }
    bool Write(cl_command_queue queue, size_t offset, size_t length, const T* host, const std::vector<cl_event>& waits)
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
        if (CL_SUCCESS != clEnqueueWriteBuffer(queue, this->mem, CL_FALSE, offset * sizeof(T), length * sizeof(T), host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
        {
            return false;
        }

        this->event = CLEvent(event);
        return true;
    }

    void Wait() const
    {
        this->event.Wait();
    }

    size_t Length() const
    {
        return this->len;
    }

    CLEvent Event() const
    {
        return this->event;
    }
    operator cl_event() const
    {
        return (cl_event)this->event;
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
                throw std::runtime_error("Invalid memory creation flag");
        }

        auto buffer = clCreateBuffer(context, mflags, length * sizeof(T), nullptr, nullptr);
        ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuffer(buffer, length);
    }

private:

private:
    cl_mem mem;
    size_t len;
    mutable CLEvent event;
};