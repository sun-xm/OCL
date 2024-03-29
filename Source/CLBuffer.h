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
        if (CL_SUCCESS == error && mem && length)
        {
            error = clRetainMemObject(mem);
            if (CL_SUCCESS == error)
            {
                this->mem = mem;
                this->len = length;
            }
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
    CLBuffer& operator=(const CLBuffer&) = delete;

    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Map(queue, flags, {});
    }
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, size_t offset, size_t length)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
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

        return CLMemMap<T>(this->mem, queue, event, map);
    }

    bool Copy(cl_command_queue queue, const CLBuffer& source)
    {
        return this->Copy(queue, source, 0, 0, this->len < source.len ? this->len : source.len);
    }
    bool Copy(cl_command_queue queue, const CLBuffer& source, size_t srcoff, size_t dstoff, size_t length)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
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
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
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
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
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
        if (CL_SUCCESS != error)
        {
            return CLBuffer(0, 0, error);
        }

        ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuffer(buffer, length, error);
    }

protected:
    cl_mem mem;
    size_t len;
    mutable cl_int  err;
    mutable CLEvent evt;
};

template<typename T>
class CLBuff2D : public CLBuffer<T>
{
public:
    CLBuff2D() : CLBuffer(), pitch(0), width(0), height(0)
    {
    }
    CLBuff2D(cl_mem mem, size_t length, size_t width, size_t height, size_t pitch, cl_int error) : CLBuffer(mem, length, error)
    {
        if (CL_SUCCESS == this->err)
        {
            this->pitch  = pitch;
            this->width  = width;
            this->height = height;
        }
        else
        {
            this->pitch  = 0;
            this->width  = 0;
            this->height = 0;
        }
    }
    CLBuff2D(CLBuff2D&& other) : CLBuff2D()
    {
        *this = std::move(other);
    }
    CLBuff2D(const CLBuff2D&) = delete;

    CLBuff2D& operator=(CLBuff2D&& other)
    {
        *(CLBuffer*)this = std::move((CLBuffer&)other);

        auto pitch  = this->pitch;
        auto width  = this->width;
        auto height = this->height;

        this->pitch  = other.pitch;
        this->width  = other.width;
        this->height = other.height;

        other.pitch  = pitch;
        other.width  = width;
        other.height = height;

        this->evt = std::move(other.evt);

        return *this;
    }
    CLBuff2D& operator=(const CLBuff2D&) = delete;

    bool Read(cl_command_queue queue, size_t x, size_t y, size_t width, size_t height, T* host, size_t hostX, size_t hostY, size_t pitch, const std::vector<cl_event>& waits)
    {
        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        size_t borigin[3] = { x * sizeof(T), y, 0 };
        size_t horigin[3] = { hostX * sizeof(T), hostY,  0 };
        size_t region[3]  = { width * sizeof(T), height, 1 };

        cl_event event;
        this->err = clEnqueueReadBufferRect(queue, this->mem, CL_FALSE, borigin, horigin, region, this->pitch, 0, pitch, 0, host,
                                            (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    bool Write(cl_command_queue queue, size_t x, size_t y, size_t width, size_t height, const T* host, size_t hostX, size_t hostY, size_t pitch)
    {
        ONCLEANUP(wait, [this]{ if (CL_SUCCESS == this->err) this->Wait(); });
        return this->Write(queue, x, y, width, height, host, hostX, hostY, pitch, {});
    }
    // Do not queue up more than 1 consecutive async rect write together on Intel platform. It's not working.
    bool Write(cl_command_queue queue, size_t x, size_t y, size_t width, size_t height, const T* host, size_t hostX, size_t hostY, size_t pitch, const std::vector<cl_event>& waits)
    {
        std::vector<cl_event> events;
        for (auto& e : waits)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        size_t borigin[3] = { x * sizeof(T), y, 0 };
        size_t horigin[3] = { hostX * sizeof(T), hostY,  0 };
        size_t region[3]  = { width * sizeof(T), height, 1 };

        cl_event event;
        this->err = clEnqueueWriteBufferRect(queue, this->mem, CL_FALSE, borigin, horigin, region, this->pitch, 0, pitch, 0, host,
                                            (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    size_t Width() const
    {
        return this->width;
    }

    size_t Height() const
    {
        return this->height;
    }

    size_t Pitch() const
    {
        return this->pitch;
    }

    static CLBuff2D Create(cl_context context, uint32_t flags, size_t width, size_t height, size_t pitch = 0)
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
            return CLBuff2D(0, 0, 0, 0, 0, error);
        }

        ONCLEANUP(buffer, [buffer]{ if (buffer) clReleaseMemObject(buffer); });
        return CLBuff2D(buffer, pitch * height, width, height, pitch, error);
    }

protected:
    size_t pitch;
    size_t width;
    size_t height;
};