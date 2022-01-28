#include "CLBuffer.h"
#include "CLCommon.h"
#include "CLFlags.h"
#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

CLBuffer::CLBuffer() : mem(nullptr)
{
}

CLBuffer::CLBuffer(cl_mem mem) : CLBuffer()
{
    if (mem && CL_SUCCESS == clRetainMemObject(mem))
    {
        this->mem = mem;
    }
}

CLBuffer::CLBuffer(CLBuffer&& other)
{
    *this = move(other);
}

CLBuffer::CLBuffer(const CLBuffer& other) : CLBuffer(nullptr)
{
    *this = other;
}

CLBuffer::~CLBuffer()
{
    if (this->mem)
    {
        clReleaseMemObject(this->mem);
    }
}

CLBuffer& CLBuffer::operator=(CLBuffer&& other)
{
    if (this->mem)
    {
        clReleaseMemObject(this->mem);
    }

    this->mem = other.mem;
    other.mem = nullptr;
    return *this;
}

CLBuffer& CLBuffer::operator=(const CLBuffer& other)
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

CLMemMap CLBuffer::Map(cl_command_queue queue, uint32_t flags)
{
    ONCLEANUP(_, [this]{ this->Event().Wait(); });
    return this->Map(queue, flags, {});
}

CLMemMap CLBuffer::Map(cl_command_queue queue, uint32_t flags, const initializer_list<CLEvent>& waitList)
{
    size_t bytes;
    if (CL_SUCCESS != clGetMemObjectInfo(this->mem, CL_MEM_SIZE,  sizeof(bytes), &bytes, nullptr))
    {
        return CLMemMap(nullptr, nullptr, nullptr, nullptr);
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
    for (auto& e : waitList)
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
        return CLMemMap(nullptr, nullptr, nullptr, nullptr);
    }

    this->event = CLEvent(event);
    clReleaseEvent(event);

    return CLMemMap(this->mem, queue, event, map);
}

size_t CLBuffer::Size() const
{
    if (!this->mem)
    {
        return 0;
    }

    size_t length;
    auto error = clGetMemObjectInfo(this->mem, CL_MEM_SIZE, sizeof(length), &length, nullptr);
    assert(CL_SUCCESS == error);

    return length;
}

CLBuffer CLBuffer::Create(cl_context context, uint32_t flags, size_t bytes)
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

    cl_int error;
    auto buffer = clCreateBuffer(context, mflags, bytes, nullptr, &error);
    ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
    return CLBuffer(buffer);
}