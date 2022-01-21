#include "CLContext.h"
#include <utility>

using namespace std;

CLContext::CLContext() : context(nullptr)
{
}

CLContext::CLContext(CLContext&& other)
{
    *this = move(other);
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

bool CLContext::Create(cl_device_id device)
{
    cl_int error;
    this->context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);

    if (CL_SUCCESS != error)
    {
        this->context = nullptr;
        return false;
    }

    return true;
}