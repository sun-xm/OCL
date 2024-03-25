#pragma once

#include "CLCommon.h"

struct CLImgDsc : cl_image_desc
{
    CLImgDsc()
    {
        *(cl_image_desc*)this = {0};
    }
    CLImgDsc(size_t width, size_t height = 1, size_t depth = 1, cl_mem buffer = 0, size_t pitch = 0, size_t slice = 0, size_t array = 0) : CLImgDsc()
    {
        if (!width)
        {
            throw std::runtime_error("Invalid width");
        }

        uint32_t dims = 1;
        if (height > 1)
        {
            dims++;
        }

        if (depth > 1)
        {
            if (height <= 1)
            {
                throw std::runtime_error("Invalid height");
            }
            dims++;
        }

        if (buffer && array)
        {
            throw std::runtime_error("Conflict buffer/array");
        }

        if (buffer && 1 != dims)
        {
            throw std::runtime_error("Invalid buffer with 2D/3D image");
        }

        if (array && 3 == dims)
        {
            throw std::runtime_error("Invalid array with 3D image");
        }

        switch (dims)
        {
            case 1:
            {
                if (array)
                {
                    this->image_type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
                }
                else if (buffer)
                {
                    this->image_type = CL_MEM_OBJECT_IMAGE1D_BUFFER;
                }
                else
                {
                    this->image_type = CL_MEM_OBJECT_IMAGE1D;
                }
                break;
            }

            case 2:
            {
                if (array)
                {
                    this->image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
                }
                else
                {
                    this->image_type = CL_MEM_OBJECT_IMAGE2D;
                }
                break;
            }

            case 3:
            {
                this->image_type = CL_MEM_OBJECT_IMAGE3D;
                break;
            }
        }

        this->image_width  = width;
        this->image_height = height;
        this->image_depth  = depth;
        this->image_row_pitch = pitch;
        this->image_slice_pitch = slice;
        this->image_array_size = array;
        this->buffer = buffer;
    }
};

struct CLImgFmt : cl_image_format
{
    CLImgFmt()
    {
        *(cl_image_format*)this = {0};
    }
    CLImgFmt(cl_channel_order order, cl_channel_type type)
    {
        this->image_channel_order = order;
        this->image_channel_data_type = type;
    }
};

class CLImage
{
public:
    CLImage() : mem(nullptr), err(0)
    {
    }
    CLImage(cl_mem image, cl_int error, const CLImgFmt& format, const CLImgDsc& descriptor) : CLImage()
    {
        if (image && CL_SUCCESS == clRetainMemObject(image))
        {
            this->mem = image;
            this->fmt = format;
            this->dsc = descriptor;
        }
        this->err = error;
    }
    CLImage(CLImage&& other) : CLImage()
    {
        *this = std::move(other);
    }
    CLImage(const CLImage&) = delete;
    virtual ~CLImage()
    {
        if (this->mem)
        {
            clReleaseMemObject(this->mem);
        }
    }

    CLImage& operator=(CLImage&& other)
    {
        auto mem = this->mem;
        auto err = this->err;
        auto fmt = this->fmt;
        auto dsc = this->dsc;

        this->mem = other.mem;
        this->err = other.err;
        this->fmt = other.fmt;
        this->dsc = other.dsc;

        other.mem = mem;
        other.err = err;
        other.fmt = fmt;
        other.dsc = dsc;

        this->evt = std::move(other.evt);

        return *this;
    }
    CLImage& operator=(const CLImage&) = delete;

    template<typename T>
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, size_t& pitch, size_t& slice)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Map<T>(queue, flags, {}, pitch, slice);
    }
    template<typename T>
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, const std::vector<cl_event>& waits, size_t& pitch, size_t& slice)
    {
        return this->Map<T>(queue, flags, { 0, 0, 0 }, { this->dsc.image_width, this->dsc.image_height, this->dsc.image_depth }, waits, pitch, slice);
    }
    template<typename T>
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, const std::vector<size_t>& origin, const std::vector<size_t>& region,
                    size_t& pitch, size_t& slice)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Map<T>(queue, flags, origin, region, pitch, slice, {});
    }
    template<typename T>
    CLMemMap<T> Map(cl_command_queue queue, uint32_t flags, const std::vector<size_t>& origin, const std::vector<size_t>& region,
                    const std::vector<cl_event>& waits, size_t& pitch, size_t& slice)
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
        auto map = clEnqueueMapImage(queue, this->mem, CL_FALSE, mflags, origin.data(), region.data(), &pitch, &slice, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event, &this->err);

        if (CL_SUCCESS != this->err)
        {
            return CLMemMap<T>();
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return CLMemMap<T>(this->mem, queue, event, map);
    }

    bool Copy(cl_command_queue queue, const CLImage& source)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Copy(queue, source, {});
    }
    bool Copy(cl_command_queue queue, const CLImage& source, const std::vector<cl_event>& waits)
    {
        std::vector<size_t> region = { this->dsc.image_width  < source.dsc.image_width  ? this->dsc.image_width  : source.dsc.image_width,
                                       this->dsc.image_height < source.dsc.image_height ? this->dsc.image_height : source.dsc.image_height,
                                       this->dsc.image_depth  < source.dsc.image_depth  ? this->dsc.image_depth  : source.dsc.image_depth };
        return this->Copy(queue, source, { 0, 0, 0 }, { 0, 0, 0 }, region, waits);
    }
    bool Copy(cl_command_queue queue, const CLImage& source, const std::vector<size_t>& srcorg, const std::vector<size_t>& dstorg,
              const std::vector<size_t>& region)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Copy(queue, source, srcorg, dstorg, region, {});
    }
    bool Copy(cl_command_queue queue, const CLImage& source, const std::vector<size_t>& srcorg, const std::vector<size_t>& dstorg,
              const std::vector<size_t>& region, const std::vector<cl_event>& waits)
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
        this->err = clEnqueueCopyImage(queue, source, this->mem, srcorg.data(), dstorg.data(), region.data(), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    bool Read(cl_command_queue queue, void* host) const
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Read(queue, host, {});
    }
    bool Read(cl_command_queue queue, void* host, const std::vector<cl_event>& waits) const
    {
        return this->Read(queue, { 0, 0, 0 }, { this->dsc.image_width, this->dsc.image_height, this->dsc.image_depth }, 0, 0, host, waits);
    }
    bool Read(cl_command_queue queue, const std::vector<size_t>& origin, const std::vector<size_t>& region,
              size_t pitch, size_t slice, void * host) const
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Read(queue, origin, region, pitch, slice, host, {});
    }
    bool Read(cl_command_queue queue, const std::vector<size_t>& origin, const std::vector<size_t>& region,
              size_t pitch, size_t slice, void* host, const std::vector<cl_event>& waits) const
    {
        size_t org[3] = { 0, 0, 0 };
        size_t rgn[3] = { 1, 1, 1 };

        for (size_t i = 0; i < (origin.size() < 3 ? origin.size() : 3); i++)
        {
            org[i] = origin[i];
        }

        for (size_t i = 0; i < (region.size() < 3 ? region.size() : 3); i++)
        {
            rgn[i] = region[i];
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
        this->err = clEnqueueReadImage(queue, this->mem, CL_FALSE, org, rgn, pitch, slice, host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        if (CL_SUCCESS != this->err)
        {
            return false;
        }

        this->evt = CLEvent(event);
        clReleaseEvent(event);

        return true;
    }

    bool Write(cl_command_queue queue, void* host)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Write(queue, host, {});
    }
    bool Write(cl_command_queue queue, void* host, const std::vector<cl_event>& waits)
    {
        return this->Write(queue, { 0, 0, 0 }, { this->dsc.image_width, this->dsc.image_height, this->dsc.image_depth }, 0, 0, host, waits);
    }
    bool Write(cl_command_queue queue, const std::vector<size_t>& origin, const std::vector<size_t>& region,
               size_t pitch, size_t slice, void* host)
    {
        ONCLEANUP(wait, [this]{ this->Wait(); });
        return this->Write(queue, origin, region, pitch, slice, host, {});
    }
    bool Write(cl_command_queue queue, const std::vector<size_t>& origin, const std::vector<size_t>& region,
               size_t pitch, size_t slice, void* host, const std::vector<cl_event>& waits)
    {
        size_t org[3] = { 0, 0, 0 };
        size_t rgn[3] = { 1, 1, 1 };

        for (size_t i = 0; i < (origin.size() < 3 ? origin.size() : 3); i++)
        {
            org[i] = origin[i];
        }

        for (size_t i = 0; i < (region.size() < 3 ? region.size() : 3); i++)
        {
            rgn[i] = region[i];
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
        this->err = clEnqueueWriteImage(queue, this->mem, CL_FALSE, org, rgn, pitch, slice, host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
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

    size_t Width() const
    {
        return this->dsc.image_width;
    }
    size_t Height() const
    {
        return this->dsc.image_height;
    }
    size_t Depth() const
    {
        return this->dsc.image_depth;
    }

    const CLImgFmt& Format() const
    {
        return this->fmt;
    }

    const CLImgDsc& Descriptor() const
    {
        return this->dsc;
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

    static CLImage Create(cl_context context, uint32_t flags, const CLImgFmt& format, const CLImgDsc& descriptor)
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
                throw std::runtime_error("Unsupported image creation flag");
        }

        cl_int error;
        auto image = clCreateImage(context, mflags, &format, &descriptor, nullptr, &error);
        ONCLEANUP(image, [=]{ if (image) clReleaseMemObject(image); });
        return CLImage(image, error, format, descriptor);
    }

protected:
    cl_mem   mem;
    CLImgFmt fmt;
    CLImgDsc dsc;

    mutable cl_int  err;
    mutable CLEvent evt;
};