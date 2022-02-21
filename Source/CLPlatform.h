#pragma once

#include <CLDevice.h>
#include <string>
#include <vector>

class CLPlatform
{
public:
    CLPlatform(cl_platform_id id) : id(id) {}

    std::string Name() const
    {
        return this->Info(CL_PLATFORM_NAME);
    }
    std::string Vendor() const
    {
        return this->Info(CL_PLATFORM_VENDOR);
    }
    std::string Version() const
    {
        return this->Info(CL_PLATFORM_VERSION);
    }
    std::string Profile() const
    {
        return this->Info(CL_PLATFORM_PROFILE);
    }
    std::string Extensions() const
    {
        return this->Info(CL_PLATFORM_EXTENSIONS);
    }

    std::vector<CLDevice> Devices() const
    {
        std::vector<CLDevice> devices;

        cl_uint num;
        if (CL_SUCCESS != clGetDeviceIDs(this->id, CL_DEVICE_TYPE_ALL, 0, nullptr, &num))
        {
            return devices;
        }

        if (num)
        {
            std::vector<cl_device_id> ids(num);
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

    operator cl_platform_id() const
    {
        return this->id;
    }

    operator bool() const
    {
        return !!this->id;
    }

    static std::vector<CLPlatform> Platforms()
    {
        std::vector<CLPlatform> platforms;

        cl_uint num;
        if (CL_SUCCESS != clGetPlatformIDs(0, nullptr, &num))
        {
            return platforms;
        }

        if (num)
        {
            std::vector<cl_platform_id> ids(num);
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

private:
    std::string Info(cl_platform_info param) const
    {
        size_t length;
        if (CL_SUCCESS != clGetPlatformInfo(this->id, param, 0, nullptr, &length))
        {
            return "";
        }

        std::string info;
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

private:
    cl_platform_id id;
};