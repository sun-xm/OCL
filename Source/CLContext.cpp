#include "CLContext.h"
#include "CLCommon.h"
#include <utility>

using namespace std;

CLContext::CLContext(cl_context context) : context(nullptr)
{
    if (context && CL_SUCCESS == clRetainContext(context))
    {
        this->context = context;
    }
}

CLContext::CLContext(CLContext&& other)
{
    *this = move(other);
}

CLContext::CLContext(const CLContext& other) : CLContext(nullptr)
{
    *this = other;
}

CLContext::~CLContext()
{
    if (this->context)
    {
        clReleaseContext(this->context);
    }
}

CLContext& CLContext::operator=(CLContext&& other)
{
    if (this->context)
    {
        clReleaseContext(this->context);
    }

    this->context = other.context;
    other.context = nullptr;
    return *this;
}

CLContext& CLContext::operator=(const CLContext& other)
{
    if (other.context && CL_SUCCESS != clRetainContext(other.context))
    {
        throw runtime_error("Failed to retain context");
    }

    if (this->context)
    {
        clReleaseContext(this->context);
    }
    
    this->context = other.context;
    return *this;
}

CLQueue CLContext::CreateQueue()
{
    return CLQueue::Create(*this);
}

CLBuffer CLContext::CreateBuffer(uint64_t flags, size_t bytes)
{
    return CLBuffer::Create(this->context, flags, bytes);
}

CLContext CLContext::Create(cl_device_id device)
{
    cl_int err;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    ONCLEANUP(context, [=]{ if (context) clReleaseContext(context); });
    return CLContext(context);
}