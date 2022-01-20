#include "CLContext.h"

CLContext::CLContext() : context(nullptr)
{
}

CLContext::~CLContext()
{
    if (this->context)
    {
        clReleaseContext(this->context);
    }
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