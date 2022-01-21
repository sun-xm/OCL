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

bool CLQueue::Read(cl_mem device, void* host, size_t bytes)
{
    return this->Read(device, host, bytes, 0, true);
}

bool CLQueue::Read(cl_mem device, void* host, size_t bytes, size_t offset, bool blocking)
{
    return CL_SUCCESS == clEnqueueReadBuffer(this->queue, device, blocking ? CL_TRUE : CL_FALSE, offset, bytes, host, 0, nullptr, nullptr);
}

bool CLQueue::Write(cl_mem device, void* host, size_t bytes)
{
    return this->Write(device, host, bytes, 0, true);
}

bool CLQueue::Write(cl_mem device, void* host, size_t bytes, size_t offset, bool blocking)
{
    return CL_SUCCESS == clEnqueueWriteBuffer(this->queue, device, blocking ? CL_TRUE : CL_FALSE, 0, bytes, host, 0, nullptr, nullptr);
}

void CLQueue::Finish()
{
    clFinish(this->queue);
}