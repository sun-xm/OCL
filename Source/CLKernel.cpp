#include "CLKernel.h"
#include <utility>

using namespace std;

CLKernel::CLKernel(cl_kernel kernel) : kernel(kernel)
{
}

CLKernel::CLKernel(CLKernel&& other)
{
    *this = move(other);
}

CLKernel::~CLKernel()
{
    if (this->kernel)
    {
        clReleaseKernel(this->kernel);
    }
}

CLKernel& CLKernel::operator=(CLKernel&& other)
{
    if (this->kernel)
    {
        clReleaseKernel(this->kernel);
    }

    this->kernel = other.kernel;
    other.kernel = nullptr;
    return *this;
}

void CLKernel::Size(const initializer_list<size_t>& global, const initializer_list<size_t>& local)
{
    this->global = global;
    this->local  = local;
}