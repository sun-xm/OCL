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

bool CLQueue::Execute(const CLKernel& kernel)
{
    cl_event event;
    auto error = clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), 0, nullptr, &event);
    if (CL_SUCCESS != error)
    {
        return false;
    }
    
    kernel.Event() = CLEvent(event);
    kernel.Event().Wait();
    return true;
}

bool CLQueue::Execute(const CLKernel& kernel, const initializer_list<CLEvent>& waitList)
{
    vector<cl_event> events;
    for (auto& e : waitList)
    {
        if (e)
        {
            events.push_back(e);
        }
    }

    cl_event event;
    if (CL_SUCCESS != clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
    {
        return false;
    }

    kernel.Event() = CLEvent(event);
    return true;
}

bool CLQueue::Map(CLBuffer& buffer, void* host)
{
    return buffer.Map(this->queue, host);
}

bool CLQueue::Map(CLBuffer& buffer, void* host, const initializer_list<CLEvent>& waitList)
{
    return buffer.Map(this->queue, host, waitList);
}

bool CLQueue::Read(const CLBuffer& buffer, void* host, size_t bytes, size_t offset)
{
    if (!this->Read(buffer, host, bytes, offset, {}))
    {
        return false;
    }

    buffer.Event().Wait();
    return true;
}

bool CLQueue::Read(const CLBuffer& buffer, void* host, size_t bytes, size_t offset, const initializer_list<CLEvent>& waitList)
{
    if (buffer.Mapped())
    {
        buffer.Event() = CLEvent();

        if (buffer.Mapped() != host)
        {
            memcpy(host, ((char*)buffer.Mapped() + offset), bytes);
        }
        return true;
    }

    vector<cl_event> events;
    for (auto& e : waitList)
    {
        if (e)
        {
            events.push_back(e);
        }
    }

    cl_event event;
    if (CL_SUCCESS != clEnqueueReadBuffer(this->queue, buffer, CL_FALSE, offset, bytes, host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
    {
        return false;
    }

    buffer.Event() = CLEvent(event);
    return true;
}

bool CLQueue::Write(CLBuffer& buffer, void* host, size_t bytes, size_t offset)
{
    if (!this->Write(buffer, host, bytes, offset, {}))
    {
        return false;
    }

    buffer.Event().Wait();
    return true;
}

bool CLQueue::Write(CLBuffer& buffer, void* host, size_t bytes, size_t offset, const initializer_list<CLEvent>& waitList)
{
    if (buffer.Mapped())
    {
        buffer.Event() = CLEvent();

        if (buffer.Mapped() != host)
        {
            memcpy(((char*)buffer.Mapped() + offset), host, bytes);
        }
        return true;
    }

    vector<cl_event> events;
    for (auto& e : waitList)
    {
        if (e)
        {
            events.push_back(e);
        }
    }

    cl_event event;
    if (CL_SUCCESS != clEnqueueWriteBuffer(this->queue, buffer, CL_FALSE, offset, bytes, host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
    {
        return false;
    }

    buffer.Event() = CLEvent(event);
    return true;
}

void CLQueue::Finish()
{
    clFinish(this->queue);
}