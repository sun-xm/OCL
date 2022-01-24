#include "CLQueue.h"
#include <utility>

using namespace std;

CLQueue::CLQueue() : queue(nullptr)
{
}

CLQueue::CLQueue(CLQueue&& other)
{
    *this = move(other);
}

CLQueue::CLQueue(const CLQueue& other) : queue(nullptr)
{
    *this = other;
}

CLQueue::~CLQueue()
{
    if (this->queue)
    {
        clReleaseCommandQueue(this->queue);
    }
}

CLQueue& CLQueue::operator=(CLQueue&& other)
{
    if (this->queue)
    {
        clReleaseCommandQueue(this->queue);
    }

    this->queue = other.queue;
    other.queue = nullptr;
    return *this;
}

CLQueue& CLQueue::operator=(const CLQueue& other)
{
    if (other.queue && CL_SUCCESS != clRetainCommandQueue(other.queue))
    {
        throw runtime_error("Failed to retain queue");
    }

    if (this->queue)
    {
        clReleaseCommandQueue(this->queue);
    }

    this->queue = other.queue;
    return *this;
}

bool CLQueue::Create(cl_context context, cl_device_id device)
{
    if (this->queue)
    {
        clReleaseCommandQueue(this->queue);
    }

    cl_int error;
#if CL_TARGET_OPENCL_VERSION >= 200
    this->queue = clCreateCommandQueueWithProperties(context, device, nullptr, &error);
#else
    this->queue = clCreateCommandQueue(context, device, 0, &error);
#endif

    if (CL_SUCCESS != error)
    {
        this->queue = nullptr;
        return false;
    }

    return true;
}

bool CLQueue::Execute(const CLKernel& kernel, bool blocking)
{
    auto error = clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), 0, nullptr, nullptr);
    if (CL_SUCCESS != error)
    {
        return false;
    }

    if (blocking)
    {
        this->Finish();
    }

    return true;
}

bool CLQueue::Map(CLBuffer& buffer, void* host, bool blocking)
{
    return !!buffer.Map(this->queue, host, blocking);
}

bool CLQueue::Read(const CLBuffer& buffer, void* host, size_t bytes, size_t offset, bool blocking)
{
    if (buffer.Mapped())
    {
        if (buffer.Mapped() == host)
        {
            return true;
        }

        memcpy(host, ((char*)buffer.Mapped() + offset), bytes);
    }

    return CL_SUCCESS == clEnqueueReadBuffer(this->queue, buffer, blocking ? CL_TRUE : CL_FALSE, offset, bytes, host, 0, nullptr, nullptr);
}

bool CLQueue::Write(CLBuffer& buffer, void* host, size_t bytes, size_t offset, bool blocking)
{
    if (buffer.Mapped())
    {
        if (buffer.Mapped() == host)
        {
            return true;
        }

        memcpy(((char*)buffer.Mapped() + offset), host, bytes);
        return true;
    }

    return CL_SUCCESS == clEnqueueWriteBuffer(this->queue, buffer, blocking ? CL_TRUE : CL_FALSE, offset, bytes, host, 0, nullptr, nullptr);
}

void CLQueue::Finish()
{
    clFinish(this->queue);
}