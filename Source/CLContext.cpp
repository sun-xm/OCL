#include "CLContext.h"
#include "CLCommon.h"
#include <utility>

using namespace std;

CLContext::CLContext() : context(nullptr)
{
}

CLContext::CLContext(cl_context context) : CLContext()
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

CLProgram CLContext::CreateProgram(const char* source, const char* options, std::string& log)
{
    size_t size;
    if (CL_SUCCESS != clGetContextInfo(this->context, CL_CONTEXT_DEVICES, 0, nullptr, &size))
    {
        log = "Failed to get context devices number";
        return CLProgram();
    }

    vector<cl_device_id> devices(size / sizeof(cl_device_id));
    if (devices.empty())
    {
        log = "No associated devices to context";
        return CLProgram();
    }

    if (CL_SUCCESS != clGetContextInfo(this->context, CL_CONTEXT_DEVICES, size, &devices[0], nullptr))
    {
        log = "Failed to get context devices";
        return CLProgram();
    }

    auto length = strlen(source);
    cl_int error;

    auto program = clCreateProgramWithSource(this->context, 1, &source, &length, &error);
    if (CL_SUCCESS != error)
    {
        log = "Failed to create program";
        return CLProgram();
    }
    ONCLEANUP(program, [=]{ clReleaseProgram(program); });

    if (CL_SUCCESS != clBuildProgram(program, (cl_uint)devices.size(), devices.data(), options, nullptr, nullptr))
    {
        log = "Failed to build program\n";

        for (auto device : devices)
        {
            cl_build_status status;
            if (CL_SUCCESS != clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr))
            {
                log += "Failed to get program build status";
                break;
            }

            if (CL_BUILD_ERROR == status)
            {
                if (CL_SUCCESS != clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size))
                {
                    log += "Failed to get program build log length";
                    break;
                }

                string err;
                err.resize(size);

                if (CL_SUCCESS != clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size, &err[0], nullptr))
                {
                    log += "Failed to get program build log";
                    break;
                }

                err.resize(err.size() - 1);
                log += err;
                break;
            }
        }

        return CLProgram();
    }

    return CLProgram(program);
}

CLProgram CLContext::CreateProgram(istream& source, const char* options, std::string& log)
{
    return this->CreateProgram(string(istreambuf_iterator<char>(source), istreambuf_iterator<char>()).c_str(), options, log);
}

CLQueue CLContext::CreateQueue()
{
    return CLQueue::Create(*this);
}

CLBuffer CLContext::CreateBuffer(uint32_t flags, size_t bytes)
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