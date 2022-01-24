#include "CLBuffer.h"
#include <cassert>
#include <stdexcept>
#include <utility>

using namespace std;

uint64_t CLBuffer::RO = CL_MEM_READ_ONLY;
uint64_t CLBuffer::WO = CL_MEM_WRITE_ONLY;
uint64_t CLBuffer::RW = CL_MEM_READ_WRITE;

CLBuffer::CLBuffer(cl_mem mem) : mem(mem), map(nullptr), que(nullptr)
{
}

CLBuffer::CLBuffer(CLBuffer&& other)
{
    *this = move(other);
}

CLBuffer::CLBuffer(const CLBuffer& other) : mem(nullptr), map(nullptr)
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

bool CLBuffer::Map(cl_command_queue queue, void* host, bool blocking)
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

    this->Unmap();

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

    cl_int error;
    this->map = clEnqueueMapBuffer(this->que, this->mem, blocking ? CL_TRUE : CL_FALSE, f, 0, bytes, 0, nullptr, nullptr, &error);

    return CL_SUCCESS == error;
}

void CLBuffer::Unmap()
{
    if (this->map)
    {
        assert(this->que);

        auto error = clEnqueueUnmapMemObject(this->que, this->mem, this->map, 0, nullptr, nullptr);
        assert(CL_SUCCESS == error);
        error = clReleaseCommandQueue(this->que);
        assert(CL_SUCCESS == error);
        this->que = nullptr;
    }
}

CLBuffer CLBuffer::Create(cl_context context, uint64_t flags, size_t bytes)
{
    cl_int error;
    auto buffer = clCreateBuffer(context, flags, bytes, nullptr, &error);
    return CLBuffer(CL_SUCCESS == error ? buffer : nullptr);
}