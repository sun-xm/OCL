#include "CLQueue.h"
#include "Common.h"
#include <utility>

using namespace std;

CLQueue::CLQueue(cl_command_queue queue) : queue(nullptr)
{
    if (queue && CL_SUCCESS == clRetainCommandQueue(queue))
    {
        this->queue = queue;
    }
}

CLQueue::CLQueue(CLQueue&& other)
{
    *this = move(other);
}

CLQueue::CLQueue(const CLQueue& other) : CLQueue(nullptr)
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

CLMemMap CLQueue::Map(CLBuffer& buffer)
{
    return buffer.Map(this->queue);
}

CLMemMap CLQueue::Map(CLBuffer& buffer, const initializer_list<CLEvent>& waitList)
{
    return buffer.Map(this->queue, waitList);
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
    clReleaseEvent(event);
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
    clReleaseEvent(event);
    return true;
}

// bool CLQueue::Read(const CLBuffer& buffer, void* host, size_t bytes, size_t offset)
// {
//     if (!this->Read(buffer, host, bytes, offset, {}))
//     {
//         return false;
//     }

//     buffer.Event().Wait();
//     return true;
// }

// bool CLQueue::Read(const CLBuffer& buffer, void* host, size_t bytes, size_t offset, const initializer_list<CLEvent>& waitList)
// {
//     if (buffer.Mapped())
//     {
//         buffer.Event() = CLEvent();

//         if (buffer.Mapped() != host)
//         {
//             memcpy(host, ((char*)buffer.Mapped() + offset), bytes);
//         }
//         return true;
//     }

//     vector<cl_event> events;
//     for (auto& e : waitList)
//     {
//         if (e)
//         {
//             events.push_back(e);
//         }
//     }

//     cl_event event;
//     if (CL_SUCCESS != clEnqueueReadBuffer(this->queue, buffer, CL_FALSE, offset, bytes, host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
//     {
//         return false;
//     }

//     buffer.Event() = CLEvent(event);
//     clReleaseEvent(event);
//     return true;
// }

// bool CLQueue::Write(CLBuffer& buffer, void* host, size_t bytes, size_t offset)
// {
//     if (!this->Write(buffer, host, bytes, offset, {}))
//     {
//         return false;
//     }

//     buffer.Event().Wait();
//     return true;
// }

// bool CLQueue::Write(CLBuffer& buffer, void* host, size_t bytes, size_t offset, const initializer_list<CLEvent>& waitList)
// {
//     if (buffer.Mapped())
//     {
//         buffer.Event() = CLEvent();

//         if (buffer.Mapped() != host)
//         {
//             memcpy(((char*)buffer.Mapped() + offset), host, bytes);
//         }
//         return true;
//     }

//     vector<cl_event> events;
//     for (auto& e : waitList)
//     {
//         if (e)
//         {
//             events.push_back(e);
//         }
//     }

//     cl_event event;
//     if (CL_SUCCESS != clEnqueueWriteBuffer(this->queue, buffer, CL_FALSE, offset, bytes, host, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event))
//     {
//         return false;
//     }

//     buffer.Event() = CLEvent(event);
//     clReleaseEvent(event);
//     return true;
// }

void CLQueue::Finish()
{
    clFinish(this->queue);
}

CLQueue CLQueue::Create(cl_context context, cl_device_id device)
{
    if (!device)
    {
        cl_uint num;
        if (CL_SUCCESS != clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(num), &num, nullptr) || !num)
        {
            return CLQueue(nullptr);
        }

        if (CL_SUCCESS != clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(device), &device, nullptr))
        {
            return CLQueue(nullptr);
        }
    }

    cl_int err;
#if CL_TARGET_OPENCL_VERSION >= 200
    auto queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
#else
    auto queue = clCreateCommandQueue(context, device, 0, &err);
#endif
    ONCLEANUP(queue, [=]{ if (queue) clReleaseCommandQueue(queue); });
    return CLQueue(queue);
}