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
    cl_int error;
    this->queue = clCreateCommandQueueWithProperties(context, device, nullptr, &error);

    if (CL_SUCCESS != error)
    {
        this->queue = nullptr;
        return false;
    }

    return true;
}

bool CLQueue::Enqueue(const CLKernel& kernel)
{
    return CL_SUCCESS == clEnqueueNDRangeKernel(this->queue, kernel, kernel.Dims(), nullptr, kernel.Global(), kernel.Local(), 0, nullptr, nullptr);
}