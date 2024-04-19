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
    template<typename T0, size_t D0>
    friend class CLBuf;

public:
    CLBuf() : mem(0), err(0), width(0), height(0), depth(0), pitch(0), slice(0)
    {
    }
    CLBuf(cl_mem mem, cl_int err, size_t width, size_t height, size_t depth, size_t pitch, size_t slice) : CLBuf()
    {
        if (CL_SUCCESS == err)
        {
            err = clRetainMemObject(mem);
            if (CL_SUCCESS == err)
            {
                this->mem    = mem;
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

    template<size_t Dim = D, typename std::enable_if<1 == Dim, size_t>::type = 0>
    size_t Length() const
    {
        return this->width;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 2 && Dim <= 3, size_t>::type = 0>
    size_t Width() const
    {
        return this->width;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 2 && Dim <= 3, size_t>::type = 0>
    size_t Height() const
    {
        return this->height;
    }

    template<size_t Dim = D, typename std::enable_if<Dim == 3, size_t>::type = 0>
    size_t Depth() const
    {
        return this->depth;
    }

    template<size_t Dim = D, typename std::enable_if<Dim >= 2 && Dim <= 3, size_t>::type = 0>
    size_t Pitch() const
    {
        return this->pitch;
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
        size_t length;
        if (!this->pitch)
        {
            length = this->width;
        }
        else if (!this->slice)
        {
            length = this->height * this->pitch;
        }
        else
        {
            length = this->depth * this->slice;
        }

        return this->Map(queue, flags, 0, length, {});
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

    template<size_t DS>
    bool Copy(cl_command_queue queue, const CLBuf<T, DS>& source, size_t srcX, size_t srcY, size_t srcZ, size_t width, size_t height, size_t depth,
              size_t dstX, size_t dstY, size_t dstZ, const std::vector<cl_event>& waits)
    {
        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        size_t srcorg[3] = { srcX  * sizeof(T), srcY,   srcZ };
        size_t dstorg[3] = { dstX  * sizeof(T), dstY,   dstZ };
        size_t region[3] = { width * sizeof(T), height, depth };

        cl_event event;
        this->err = clEnqueueCopyBufferRect(queue, source.mem, this->mem, srcorg, dstorg, region, source.pitch, source.slice,
                                            this->pitch, this->slice, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    // Copy 1d to 1d
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuf<T, 1>& source)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err ) this->Wait(); });
        return this->Copy(queue, source, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuf<T, 1>& source, const std::vector<cl_event>& waits)
    {
        auto length = this->Length() < source.Length() ? this->Length() : source.Length();
        return this->Copy(queue, source, 0, length, 0, waits);
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuf<T, 1>& source, size_t srcoff, size_t length, size_t dstoff)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, source, srcoff, length, dstoff, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuf<T, 1>& source, size_t srcoff, size_t length, size_t dstoff, const std::vector<cl_event>& waits)
    {
        if (source.Length() < srcoff + length || this->Length() < dstoff + length)
        {
            this->err = CL_INVALID_VALUE;
            return false;
        }

        return this->Copy(queue, source, srcoff, 0, 0, length, 1, 1, dstoff, 0, 0, waits);
    }

    // Copy 2d/3d to 1d
    template<size_t Ds, size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuf<T, Ds>& source, size_t srcX, size_t srcY, size_t srcZ,
              size_t width, size_t height, size_t depth)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, source, srcX, srcY, srcZ, width, height, depth, {});
    }
    template<size_t Ds, size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuf<T, Ds>& source, size_t srcX, size_t srcY, size_t srcZ,
              size_t width, size_t height, size_t depth, const std::vector<cl_event>& waits)
    {
        if (!width || !height || !depth)
        {
            this->err = CL_INVALID_VALUE;
            return false;
        }

        if (this->Length() < width * height * depth)
        {
            this->err = CL_INVALID_OPERATION;
            return false;
        }

        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        size_t srcorg[3] = { srcX * sizeof(T), srcY, srcZ };
        size_t dstorg[3] = { 0, 0, 0 };
        size_t region[3] = { width * sizeof(T), height, depth };

        size_t pitch = source.width  * sizeof(T);
        size_t slice = source.height * pitch;

        cl_event event;
        this->err = clEnqueueCopyBufferRect(queue, source, this->mem, srcorg, dstorg, region, source.pitch, source.slice, pitch, slice,
                                            (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
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
            return CLBuf<T, 1>(0, error, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuf<T, 1>(buffer, error, length, 1, 1, length * sizeof(T), length * sizeof(T));
    }

    template<size_t Dim = D, typename std::enable_if<2 == Dim, CLBuf<T, 2>>::type* = 0>
    static CLBuf<T, 2> Create(cl_context context, int32_t flags, size_t width, size_t height, size_t pitch)
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
            return CLBuf<T, 2>(0, error, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [buffer]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuf<T, 2>(buffer, error, width, height, 1, pitch, height * pitch);
    }

    template<size_t Dim = D, typename std::enable_if<3 == Dim, CLBuf<T, 3>>::type* = 0>
    static CLBuf<T, 3> Create(cl_context context, int32_t flags, size_t width, size_t height, size_t depth, size_t pitch, size_t slice)
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
            return CLBuf<T, 3>(0, error, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [buffer]{ if(buffer) clReleaseMemObject(buffer); });
        return CLBuf<T, 3>(buffer, error, width, height, depth, pitch, slice);
    }

protected:
    cl_mem mem;
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