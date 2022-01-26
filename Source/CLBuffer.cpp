#include "CLBuffer.h"
#include "Common.h"
#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

uint64_t CLBuffer::RO = CL_MEM_READ_ONLY;
uint64_t CLBuffer::WO = CL_MEM_WRITE_ONLY;
uint64_t CLBuffer::RW = CL_MEM_READ_WRITE;

CLBuffer::CLBuffer(cl_mem mem) : mem(nullptr), map(nullptr), que(nullptr)
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
    this->Unmap();

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

bool CLBuffer::Map(cl_command_queue queue)
{
    if (!this->Map(queue, {}))
    {
        return false;
    }

    this->event.Wait();
    return true;
}

bool CLBuffer::Map(cl_command_queue queue, const initializer_list<CLEvent>& waitList)
{
    cl_mem_flags flags;
    if (CL_SUCCESS != clGetMemObjectInfo(this->mem, CL_MEM_FLAGS, sizeof(flags), &flags, nullptr))
    {
        return false;
    }

    size_t bytes;
    if (CL_SUCCESS != clGetMemObjectInfo(this->mem, CL_MEM_SIZE, sizeof(bytes), &bytes, nullptr))
    {
        return false;
    }

    this->Unmap(waitList);
    this->event.Wait();

    if (CL_SUCCESS != clRetainCommandQueue(queue))
    {
        return false;
    }
    this->que = queue;

    cl_map_flags f = 0;
    if (CL_MEM_READ_ONLY & flags)
    {
        f |= CL_MAP_READ;
    }
    if(CL_MEM_WRITE_ONLY & flags)
    {
        f |= CL_MAP_WRITE;
    }
    if (CL_MEM_READ_WRITE & flags)
    {
        f = CL_MAP_READ | CL_MAP_WRITE;
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
    this->map = clEnqueueMapBuffer(this->que, this->mem, CL_FALSE, f, 0, bytes, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event, &error);

    this->event = CLEvent(event);
    clReleaseEvent(event);
    return CL_SUCCESS == error;
}

void CLBuffer::Unmap(const initializer_list<CLEvent>& waitList)
{
    if (this->map)
    {
        assert(this->que);

        vector<cl_event> events;
        for (auto& e : waitList)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        cl_event event;
        auto error = clEnqueueUnmapMemObject(this->que, this->mem, this->map, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        assert(CL_SUCCESS == error);
        error = clReleaseCommandQueue(this->que);
        assert(CL_SUCCESS == error);
        this->que = nullptr;

        this->event = CLEvent(event);
        clReleaseEvent(event);
    }
    else
    {
        this->event = CLEvent();
    }
}

size_t CLBuffer::Length() const
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

CLBuffer CLBuffer::Create(cl_context context, uint64_t flags, size_t bytes)
{
    cl_int error;
    auto buffer = clCreateBuffer(context, flags, bytes, nullptr, &error);
    ONCLEANUP(buffer, [=]{ if (buffer) clReleaseMemObject(buffer); });
    return CLBuffer(buffer);
}