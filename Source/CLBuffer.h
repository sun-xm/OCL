#pragma once

#include "CLCommon.h"
#include "CLFlags.h"
#include "CLMemMap.h"
#include <cstdint>
#include <stdexcept>
#include <utility>

template<typename T, size_t D = 1>
class CLBuffer
{
    template<typename T0, size_t D0>
    friend class CLBuffer;

public:
    CLBuffer() : mem(0), err(0), width(0), height(0), depth(0), pitch(0), slice(0)
    {
    }
    CLBuffer(cl_mem mem, cl_int err, size_t width, size_t height, size_t depth, size_t pitch, size_t slice) : CLBuffer()
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
    CLBuffer(CLBuffer&& other) : CLBuffer()
    {
        *this = std::forward<CLBuffer>(other);
    }
    CLBuffer(const CLBuffer&) = delete;
    virtual ~CLBuffer()
    {
        if (this->mem)
        {
            clReleaseMemObject(this->mem);
        }
    }

    CLBuffer& operator=(CLBuffer&& other)
    {
        std::swap(this->mem,    other.mem);
        std::swap(this->err,    other.err);
        std::swap(this->evt,    other.evt);
        std::swap(this->width,  other.width);
        std::swap(this->height, other.height);
        std::swap(this->depth,  other.depth);
        std::swap(this->pitch,  other.pitch);
        std::swap(this->slice,  other.slice);
        return *this;
    }
    CLBuffer& operator=(const CLBuffer&) = delete;

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

    // Map functions
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

    // General copy
    bool Copy(cl_command_queue queue, const CLBuffer& src, size_t srcX, size_t srcY, size_t srcZ, size_t width, size_t height, size_t depth,
              size_t dstX, size_t dstY, size_t dstZ, const std::vector<cl_event>& waits)
    {
        if (srcX + width  > src.width    ||
            srcY + height > src.height   ||
            srcZ + depth  > src.depth    ||
            dstX + width  > this->width  ||
            dstY + height > this->height ||
            dstZ + depth  > this->depth)
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

        size_t srcorg[3] = { srcX  * sizeof(T), srcY,   srcZ };
        size_t dstorg[3] = { dstX  * sizeof(T), dstY,   dstZ };
        size_t region[3] = { width * sizeof(T), height, depth };

        cl_event event;
        this->err = clEnqueueCopyBufferRect(queue, src.mem, this->mem, srcorg, dstorg, region, src.pitch, src.slice,
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
    bool Copy(cl_command_queue queue, const CLBuffer<T, 1>& src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err ) this->Wait(); });
        return this->Copy(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 1>& src, const std::vector<cl_event>& waits)
    {
        auto length = this->Length() < src.Length() ? this->Length() : src.Length();
        return this->Copy(queue, src, 0, length, 0, waits);
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 1>& src, size_t srcoff, size_t length, size_t dstoff)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, src, srcoff, length, dstoff, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 1>& src, size_t srcoff, size_t length, size_t dstoff, const std::vector<cl_event>& waits)
    {
        return this->Copy(queue, src, srcoff, 0, 0, length, 1, 1, dstoff, 0, 0, waits);
    }

    // Copy 2d to 2d
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 2>& src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 2>& src, const std::vector<cl_event>& waits)
    {
        auto wdith  = this->width  < src.width  ? this->width  : src.width;
        auto height = this->height < src.height ? this->height : src.height;
        return this->Copy(queue, src, 0, 0, width, height, 0, 0, waits);
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 2>& src, size_t srcX, size_t srcY, size_t width, size_t height, size_t dstX, size_t dstY)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, src, srcX, srcY, width, height, dstX, dstY, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 2>& src, size_t srcX, size_t srcY, size_t width, size_t height,
              size_t dstX, size_t dstY, const std::vector<cl_event>& waits)
    {
        return this->Copy(queue, src, srcX, srcY, 0, width, height, 1, dstX, dstY, 0, waits);
    }

    // Copy 3d to 3d
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 3>& src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait();});
        return this->Copy(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 3>& src, const std::vector<cl_event>& waits)
    {
        auto width  = this->width  < src.width  ? this->width  : src.width;
        auto height = this->height < src.height ? this->height : src.height;
        auto depth  = this->depth  < src.depth  ? this->depth  : src.depth;
        return this->Copy(queue, src, 0, 0, 0, width, height, depth, 0, 0, 0, waits);
    }
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 3>& src, size_t srcX, size_t srcY, size_t srcZ,
              size_t width, size_t height, size_t depth, size_t dstX, size_t dstY, size_t dstZ)
    {
        ONCLENAUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, srcX, srcY, srcZ, width, height, depth, dstX, dstY, dstZ, {});
    }

    // Copy 2d/3d whole to 1d
    template<size_t Ds, size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, Ds>& src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, src, {});
    }
    template<size_t Ds, size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, Ds>& src, const std::vector<cl_event>& waits)
    {
        if (this->Length() != src.width * src.height * src.depth)
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

        size_t srcorg[3] = { 0, 0, 0 };
        size_t dstorg[3] = { 0, 0, 0 };
        size_t region[3] = { src.width * sizeof(T), src.height, src.depth };

        size_t pitch = region[0];
        size_t slice = src.height * pitch;

        cl_event event;
        this->err = clEnqueueCopyBufferRect(queue, src, this->mem, srcorg, dstorg, region, src.pitch, src.slice, pitch, slice,
                                            (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    // Copy 1d whole to 2d/3d
    template<size_t Dim = D, typename std::enable_if<2 == Dim || 3 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 1>& src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Copy(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim || 3 == Dim, bool>::type = 0>
    bool Copy(cl_command_queue queue, const CLBuffer<T, 1>& src, const std::vector<cl_event>& waits)
    {
        if (src.Length() != this->width * this->height * this->depth)
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

        size_t srcorg[3] = { 0, 0, 0 };
        size_t dstorg[3] = { 0, 0, 0 };
        size_t region[3] = { this->width * sizeof(T), this->height, this->depth };

        size_t pitch = region[0];
        size_t slice = this->height * pitch;

        cl_event event;
        this->err = clEnqueueCopyBufferRect(queue, src, this->mem, srcorg, dstorg, region, pitch, slice, this->pitch, this->slice,
                                            (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    // General read
    bool Read(cl_command_queue queue, size_t srcX, size_t srcY, size_t srcZ, size_t width, size_t height, size_t depth, T* dst,
              size_t dstX, size_t dstY, size_t dstZ, size_t pitch, size_t slice, const std::vector<cl_event>& waits) const
    {
        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        size_t srcorg[3] = { srcX * sizeof(T), srcY, srcZ };
        size_t dstorg[3] = { dstX * sizeof(T), dstY, dstZ };
        size_t region[3] = { width * sizeof(T), height, depth };

        if (0 == pitch)
        {
            pitch = width * sizeof(T);
        }

        if (0 == slice)
        {
            slice = height * pitch;
        }

        cl_event event;
        this->err = clEnqueueReadBufferRect(queue, this->mem, CL_FALSE, srcorg, dstorg, region, this->pitch, this->slice, pitch, slice,
                                            dst, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    // Read 1d
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, T* dst) const
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Read(queue, dst, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, T* dst, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, 0, this->Length(), dst, waits);
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, size_t offset, size_t length, T* dst) const
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Read(queue, offset, length, dst, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, size_t offset, size_t length, T* dst, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, offset, 0, 0, length, 1, 1, dst, 0, 0, 0, 0, 0, waits);
    }

    // Read 2d
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, T* dst) const
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Read(queue, dst, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, T* dst, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, 0, 0, this->width, this->height, dst, 0, 0, 0);
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, size_t srcX, size_t srcY, size_t width, size_t height, T* dst,
              size_t dstX, size_t dstY, size_t pitch) const
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Read(queue, srcX, srcY, width, height, dst, dstX, dstY, pitch, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, size_t srcX, size_t srcY, size_t width, size_t height, T* dst,
              size_t dstX, size_t dstY, size_t pitch, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, srcX, srcY, 0, width, height, 1, dst, dstX, dstY, 0, pitch, 0, waits);
    }

    // Read 3d
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, T* dst) const
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Read(queue, dst, {});
    }
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, T* dst, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, 0, 0, 0, this->width, this->height, this->depth, dst, 0, 0, 0, 0, 0, waits);
    }
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Read(cl_command_queue queue, size_t srcX, size_t srcY, size_t srcZ, size_t width, size_t height, size_t depth, T* dst,
              size_t dstX, size_t dstY, size_t dstZ, size_t pitch, size_t slice, const std::vector<cl_event>& waits) const
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Read(queue, srcX, srcY, srcZ, width, height, depth, dst, dstX, dstY, dstZ, pitch, slice, {});
    }

    // General write
    bool Write(cl_command_queue queue, size_t dstX, size_t dstY, size_t dstZ, size_t width, size_t height, size_t depth, const T* src,
               size_t srcX, size_t srcY, size_t srcZ, size_t pitch, size_t slice, const std::vector<cl_event>& waits)
    {
        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        size_t srcorg[3] = { srcX * sizeof(T), srcY, srcZ };
        size_t dstorg[3] = { dstX * sizeof(T), dstY, dstZ };
        size_t region[3] = { width * sizeof(T), height, depth};

        if (0 == pitch)
        {
            pitch = width * sizeof(T);
        }

        if (0 == slice)
        {
            slice = height * pitch;
        }

        cl_event event;
        this->err = clEnqueueWriteBufferRect(queue, this->mem, CL_FALSE, dstorg, srcorg, region, this->pitch, this->slice, pitch, slice,
                                             src, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    // Async write may not working on Intel platform. clEnqueueWriteBufferRect seems cannot handle wait list properly.
    // Write 1d
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, const T* src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, const T* src, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, 0, this->Length(), src, waits);
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, size_t offset, size_t length, const T* src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, offset, length, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<1 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, size_t offset, size_t length, const T* src, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, offset, 0, 0, length, 1, 1, src, 0, 0, 0, 0, 0, waits);
    }

    // Write 2d
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, const T* src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, const T* src, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, 0, 0, this->width, this->height, src, 0, 0, 0, waits);
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, size_t dstX, size_t dstY, size_t width, size_t height, const T* src,
               size_t srcX, size_t srcY, size_t pitch)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, dstX, dstY, width, height, src, srcX, srcY, pitch, {});
    }
    template<size_t Dim = D, typename std::enable_if<2 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, size_t dstX, size_t dstY, size_t width, size_t height, const T* src,
               size_t srcX, size_t srcY, size_t pitch, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, dstX, dstY, 0, width, height, 1, src, srcX, srcY, 0, pitch, 0, waits);
    }

    // Write 3d
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, const T* src)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, src, {});
    }
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, const T* src, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, 0, 0, 0, this->width, this->height, this->depth, src, 0, 0, 0, 0, 0);
    }
    template<size_t Dim = D, typename std::enable_if<3 == Dim, bool>::type = 0>
    bool Write(cl_command_queue queue, size_t dstX, size_t dstY, size_t dstZ, size_t width, size_t height, size_t depth, const T* src,
               size_t srcX, size_t srcY, size_t srcZ, size_t pitch, size_t slice)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, dstX, dstY, dstZ, width, height, depth, src, srcX, srcY, srcZ, pitch, slice, {});
    }

    // Create functions
    template<size_t Dim = D, typename std::enable_if<1 == Dim, CLBuffer<T, 1>>::type* = 0>
    static CLBuffer<T, 1> Create(cl_context context, int32_t flags, size_t length)
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
            return CLBuffer<T, 1>(0, error, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuffer<T, 1>(buffer, error, length, 1, 1, length * sizeof(T), length * sizeof(T));
    }

    template<size_t Dim = D, typename std::enable_if<2 == Dim, CLBuffer<T, 2>>::type* = 0>
    static CLBuffer<T, 2> Create(cl_context context, int32_t flags, size_t width, size_t height, size_t pitch = 0)
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
            return CLBuffer<T, 2>(0, error, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [buffer]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuffer<T, 2>(buffer, error, width, height, 1, pitch, height * pitch);
    }

    template<size_t Dim = D, typename std::enable_if<3 == Dim, CLBuffer<T, 3>>::type* = 0>
    static CLBuffer<T, 3> Create(cl_context context, int32_t flags, size_t width, size_t height, size_t depth, size_t pitch = 0, size_t slice = 0)
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
            return CLBuffer<T, 3>(0, error, 0, 0, 0, 0, 0);
        }

        ONCLEANUP(buffer, [buffer]{ if(buffer) clReleaseMemObject(buffer); });
        return CLBuffer<T, 3>(buffer, error, width, height, depth, pitch, slice);
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
using CLBuff2D = CLBuffer<T, 2>;

template<typename T>
using CLBuff3D = CLBuffer<T, 3>;