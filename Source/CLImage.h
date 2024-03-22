#pragma once

#include "CLCommon.h"

struct CLImgDsc : cl_image_desc
{
    CLImgDsc(size_t width, size_t height = 0, size_t depth = 0, cl_mem buffer = 0, size_t pitch = 0, size_t slice = 0, size_t array = 0)
    {
        *(cl_image_desc*)this = {0};

        if (!width)
        {
            throw std::runtime_error("Invalid width");
        }

        uint32_t dims = 1;
        if (height)
        {
            dims++;
        }

        if (depth)
        {
            if (!height)
            {
                throw std::runtime_error("Invalid depth");
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
    CLImage(cl_mem image, cl_int error) : CLImage()
    {
        if (image && CL_SUCCESS == clRetainMemObject(image))
        {
            this->mem = image;
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

        this->mem = other.mem;
        this->err = other.err;

        other.mem = mem;
        other.err = err;

        this->evt = std::move(other.evt);

        return *this;
    }
    CLImage& operator=(const CLImage&) = delete;

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
        return CLImage(image, error);
    }

protected:
    cl_mem mem;
    mutable cl_int  err;
    mutable CLEvent evt;
};