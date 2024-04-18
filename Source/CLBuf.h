#pragma once

#include "CLCommon.h"
#include "CLFlags.h"
#include "CLMemMap.h"
#include <cstdint>
#include <stdexcept>
#include <utility>

template<typename T, size_t D = 1>
class CLBuf
{
public:
    CLBuf() : mem(0), err(0), length(0), width(0), height(0), depth(0), pitch(0), slice(0)
    {
    }
    CLBuf(cl_mem mem, cl_int err, size_t length, size_t width, size_t height, size_t pitch, size_t depth, size_t slice) : CLBuf()
    {
        if (CL_SUCCESS == err)
        {
            err = clRetainMemObject(mem);
            if (CL_SUCCESS == err)
            {
                this->mem    = mem;
                this->length = length;
                this->width  = width;
                this->height = height;
                this->pitch  = pitch;
                this->depth  = depth;
                this->slice  = slice;
            }
        }
        this->err = err;
    }
    CLBuf(CLBuf&& other)
    {
        *this = std::forward(other);
    }
    CLBuf(const CLBuf&) = delete;
    virtual ~CLBuf()
    {
        if (this->mem)
        {
            clReleaseMemObject(this->mem);
        }
    }

    CLBuf& operator=(CLBuf&& other)
    {
        std::swap(this->mem,    other.mem);
        std::swap(this->err,    other.err);
        std::swap(this->evt,    other.evt);
        std::swap(this->length, other.length);
        std::swap(this->width,  other.width);
        std::swap(this->height, other.height);
        std::swap(this->depth,  other.depth);
        std::swap(this->pitch,  other.pitch);
        std::swap(this->slice,  other.slice);
    }
    CLBuf& operator=(const CLBuf&) = delete;

    operator bool() const
    {
        return !!this->mem;
    }
    operator cl_mem() const
    {
        return this->mem;
    }
    operator cl_event() const
    {
        return (cl_event)this->evt;
    }

    void Wait() const
    {
        this->err = this->evt.Wait();
    }

    cl_int Error() const
    {
        return this->err;
    }

    CLEvent Event() const
    {
        return this->evt;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 1 && Dim <= 3, size_t>::type = 0>
    size_t Length() const
    {
        return this->length;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 2 && Dim <= 3, size_t>::type = 0>
    size_t Width() const
    {
        return this->width;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 2 && Dim <= 3, size_t>::type = 0>
    size_t Height() const
    {
        return this->width;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 2 && Dim <= 3, size_t>::type = 0>
    size_t Pitch() const
    {
        return this->width;
    }

    template<size_t Dim = D, typename std::enable_if<3 == Dim, size_t>::type = 0>
    size_t Depth() const
    {
        return this->depth;
    }

    template<size_t Dim = D, typename std::enable_if<3 == Dim, size_t>::type = 0>
    size_t Slice() const
    {
        return this->slice;
    }

    CLMemMap<T> Map(cl_command_queue queue, int32_t flags)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Map(queue, flags, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, int32_t flags, const std::vector<cl_event>& waits)
    {
        return this->Map(queue, flags, 0, this->length, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, int32_t flags, size_t offset, size_t length)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Map(queue, flags, offset, length, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, int32_t flags, size_t offset, size_t length, const std::vector<cl_event>& waits)
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

        return CLMemMap<T>(this->mem, queue, event, map);
    }

    template<size_t Dim = D, typename std::enable_if<1 == Dim, CLBuf<T, 1>>::type* = 0>
    static CLBuf<T, 1> Create(cl_context context, int32_t flags, size_t length)
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
        if (CL_SUCCESS != error)
        {
            return CLBuf<T, 1>(0, error, 0, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuf<T, 1>(buffer, error, length, 0, 0, 0, 0, 0);
    }

    template<size_t Dim = D, typename std::enable_if<2 == Dim, CLBuf<T, 2>>::type* = 0>
    static CLBuf<T, 2> Create(cl_context context, int32_t flags, size_t width, size_t height, size_t pitch = 0)
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

        if (0 == pitch)
        {
            auto align = CLContext(context).Device().MemBaseAddrAlign();
            pitch = (width * sizeof(T) + align - 1) / align * align;
        }

        cl_int error;
        auto buffer = clCreateBuffer(context, mflags, pitch * height, nullptr, &error);
        if (CL_SUCCESS != error)
        {
            return CLBuf<T, 2>(0, error, 0, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [buffer]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuf<T, 2>(buffer, error, pitch * height, width, height, pitch, 0, 0);
    }

    template<size_t Dim = D, typename std::enable_if<3 == Dim, CLBuf<T, 3>>::type* = 0>
    static CLBuf<T, 3> Create(cl_context context, int32_t flags, size_t width, size_t height, size_t depth, size_t pitch = 0, size_t slice = 0)
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

        if (0 == pitch)
        {
            auto align = CLContext(context).Device().MemBaseAddrAlign();
            pitch = (width * sizeof(T) + align - 1) / align * align;
        }

        if (0 == slice)
        {
            slice = pitch * height;
        }

        cl_int error;
        auto buffer = clCreateBuffer(context, mflags, slice * depth, nullptr, &error);
        if (CL_SUCCESS != error)
        {
            return CLBuf<T, 3>(0, error, 0, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [buffer]{ if(buffer) clReleaseMemObject(buffer); });
        return CLBuf<T, 3>(buffer, error, slice * depth, width, height, pitch, depth, slice);
    }

protected:
    cl_mem mem;
    size_t length;
    size_t width;
    size_t height;
    size_t depth;
    size_t pitch;
    size_t slice;

    mutable cl_int  err;
    mutable CLEvent evt;
};

template<typename T>
using CLB2D = CLBuf<T, 2>;

template<typename T>
using CLB3D = CLBuf<T, 3>;