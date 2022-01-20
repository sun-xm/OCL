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