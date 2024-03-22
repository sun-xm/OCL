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
    CLBuffer() : mem(nullptr), len(0), err(0)
    {
    }
    CLBuffer(cl_mem mem, size_t length, cl_int error) : CLBuffer()
    {
        if (mem && length && CL_SUCCESS == clRetainMemObject(mem))
        {
            this->mem = mem;
            this->len = length;
        }
        this->err = error;
    }
    CLBuffer(CLBuffer&& other) : CLBuffer()
    {
        *this = std::move(other);
    }
    CLBuffer(const CLBuffer& other) = delete;
    virtual ~CLBuffer()
    {
        if (this->mem)
        {
            clReleaseMemObject(this->mem);
        }
    }

    CLBuffer& operator=(CLBuffer&& other)
    {
        auto mem = this->mem;
        auto len = this->len;
        auto err = this->err;

        this->mem = other.mem;
        this->len = other.len;
        this->err = other.err;

        other.mem = mem;
        other.len = len;
        other.err = err;

        this->evt = std::move(other.evt);

        return *this;
    }
    CLBuffer& operator=(const CLBuffer& other) = delete;

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

        cl_event event;
        auto map = clEnqueueMapBuffer(queue, this->mem, CL_FALSE, mflags, offset * sizeof(T), length * sizeof(T), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event, &this->err);

        if (CL_SUCCESS != this->err)
        {
            return CLMemMap<T>();
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return CLMemMap<T>(this->mem, queue, event, map, length);
    }

    bool Copy(cl_command_queue queue, const CLBuffer& source)
    {
        return this->Copy(queue, source, 0, 0, this->len < source.len ? this->len : source.len);
    }
    bool Copy(cl_command_queue queue, const CLBuffer& source, size_t srcoff, size_t dstoff, size_t length)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Copy(queue, source, srcoff, dstoff, length, {});
    }
    bool Copy(cl_command_queue queue, const CLBuffer& source, const std::vector<cl_event>& waits)
    {
        return this->Copy(queue, source, 0, 0, this->len < source.len ? this->len : source.len, waits);
    }
    bool Copy(cl_command_queue queue, const CLBuffer& source, size_t srcoff, size_t dstoff, size_t length, const std::vector<cl_event>& waits)
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
        this->err = clEnqueueCopyBuffer(queue, source, this->mem, srcoff * sizeof(T), dstoff * sizeof(T), length * sizeof(T), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

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
        this->err = clEnqueueReadBuffer(queue, this->mem, CL_FALSE, offset * sizeof(T), length * sizeof(T), host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

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
        this->err = clEnqueueWriteBuffer(queue, this->mem, CL_FALSE, offset * sizeof(T), length * sizeof(T), host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    void Wait() const
    {
        this->err = this->evt.Wait();
    }

    size_t Length() const
    {
        return this->len;
    }

    cl_int Error() const
    {
        return this->err;
    }

    CLEvent Event() const
    {
        return this->evt;
    }
    operator cl_event() const
    {
        return (cl_event)this->evt;
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
                throw std::runtime_error("Unsupported memory creation flag");
        }

        cl_int error;
        auto buffer = clCreateBuffer(context, mflags, length * sizeof(T), nullptr, &error);
        ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuffer(buffer, length, error);
    }

protected:
    cl_mem mem;
    size_t len;
    mutable cl_int  err;
    mutable CLEvent evt;
};