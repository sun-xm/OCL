#include "CLPlatform.h"

using namespace std;

CLPlatform::CLPlatform(cl_platform_id id) : id(id)
{
}

string CLPlatform::Name() const
{
    return this->Info(CL_PLATFORM_NAME);
}

string CLPlatform::Vendor() const
{
    return this->Info(CL_PLATFORM_VENDOR);
}

string CLPlatform::Version() const
{
    return this->Info(CL_PLATFORM_VERSION);
}

string CLPlatform::Profile() const
{
    return this->Info(CL_PLATFORM_PROFILE);
}

string CLPlatform::Extensions() const
{
    return this->Info(CL_PLATFORM_EXTENSIONS);
}

vector<CLDevice> CLPlatform::Devices() const
{
    vector<CLDevice> devices;

    cl_uint num;
    if (CL_SUCCESS != clGetDeviceIDs(this->id, CL_DEVICE_TYPE_ALL, 0, nullptr, &num))
    {
        return devices;
    }

    if (num)
    {
        vector<cl_device_id> ids(num);
        if (CL_SUCCESS != clGetDeviceIDs(this->id, CL_DEVICE_TYPE_ALL, num, &ids[0], nullptr))
        {
            return devices;
        }

        devices.reserve(num);
        for (auto id : ids)
        {
            devices.push_back(CLDevice(id));
        }
    }

    return devices;
}

vector<CLPlatform> CLPlatform::Platforms()
{
    vector<CLPlatform> platforms;

    cl_uint num;
    if (CL_SUCCESS != clGetPlatformIDs(0, nullptr, &num))
    {
        return platforms;
    }

    if (num)
    {
        vector<cl_platform_id> ids(num);
        if (CL_SUCCESS != clGetPlatformIDs(num, &ids[0], nullptr))
        {
            return platforms;
        }

        platforms.reserve(num);
        for (auto id : ids)
        {
            platforms.push_back(CLPlatform(id));
        }
    }

    return platforms;
}

string CLPlatform::Info(cl_platform_info param) const
{
    size_t length;
    if (CL_SUCCESS != clGetPlatformInfo(this->id, param, 0, nullptr, &length))
    {
        return "";
    }

    string info;
    info.resize(length);

    if (CL_SUCCESS == clGetPlatformInfo(this->id, param, length, &info[0], nullptr))
    {
        info.resize(length - 1);
    }
    else
    {
        info.clear();
    }

    return info;
}