#include "CLProgram.h"
#include <vector>
#include <utility>

using namespace std;

CLProgram::CLProgram() : program(nullptr)
{
}

CLProgram::CLProgram(CLProgram&& other)
{
    *this = move(other);
}

CLProgram::CLProgram(const CLProgram& other) : CLProgram()
{
    *this = other;
}

CLProgram::~CLProgram()
{
    if (this->program)
    {
        clReleaseProgram(this->program);
    }
}

CLProgram& CLProgram::operator=(CLProgram&& other)
{
    if (this->program)
    {
        clReleaseProgram(this->program);
    }

    this->program = other.program;
    other.program = nullptr;
    return *this;
}

CLProgram& CLProgram::operator=(const CLProgram& other)
{
    if (other.program && CL_SUCCESS != clRetainProgram(other.program))
    {
        throw runtime_error("Failed to retain program");
    }

    if (this->program)
    {
        clReleaseProgram(this->program);
    }

    this->program = other.program;
    return *this;
}

bool CLProgram::Create(cl_context context, const string& source, const string& options, string& log)
{
    size_t size;
    if (CL_SUCCESS != clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, nullptr, &size))
    {
        log = "Failed to get context devices number";
        return false;
    }

    vector<cl_device_id> devices(size / sizeof(cl_device_id));
    if (devices.empty())
    {
        log = "No associated devices to context";
        return false;
    }

    if (CL_SUCCESS != clGetContextInfo(context, CL_CONTEXT_DEVICES, size, &devices[0], nullptr))
    {
        log = "Failed to get context devices";
        return false;
    }

    auto s = source.c_str();
    auto l = source.length();
    cl_int error;

    this->program = clCreateProgramWithSource(context, 1, &s, &l, &error);
    if (CL_SUCCESS != error)
    {
        log = "Failed to create program";
        this->program = nullptr;
        return false;
    }

    if (CL_SUCCESS != clBuildProgram(this->program, (cl_uint)devices.size(), devices.data(), options.c_str(), nullptr, nullptr))
    {
        log = "Failed to build program\n";

        for (auto device : devices)
        {
            cl_build_status status;
            if (CL_SUCCESS != clGetProgramBuildInfo(this->program, device, CL_PROGRAM_BUILD_STATUS, sizeof(status), &status, nullptr))
            {
                log += "Failed to get program build status";
                break;
            }

            if (CL_BUILD_ERROR == status)
            {
                if (CL_SUCCESS != clGetProgramBuildInfo(this->program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size))
                {
                    log += "Failed to get program build log length";
                    break;
                }

                string err;
                err.resize(size);

                if (CL_SUCCESS != clGetProgramBuildInfo(this->program, device, CL_PROGRAM_BUILD_LOG, size, &err[0], nullptr))
                {
                    log += "Failed to get program build log";
                    break;
                }

                err.resize(err.size() - 1);
                log += err;
                break;
            }
        }

        this->program = nullptr;
        return false;
    }

    return true;
}

bool CLProgram::Create(cl_context context, istream& source, const string& options, string& log)
{
    return this->Create(context, string(istreambuf_iterator<char>(source), istreambuf_iterator<char>()), options, log);
}

CLKernel CLProgram::CreateKernel(const string& name)
{
    return CLKernel::Create(this->program, name);
}