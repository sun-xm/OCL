#pragma once

#include <CL/cl.h>
#include <stdexcept>
#include <string>

class CLDevice
{
public:
    CLDevice(cl_device_id id) : id(id) {}

    std::string Name() const
    {
        std::string name;
        this->Info(CL_DEVICE_NAME, name);
        return name;
    }
    std::string Driver() const
    {
        std::string driver;
        this->Info(CL_DRIVER_VERSION, driver);
        return driver;
    }
    std::string Vendor() const
    {
        std::string vendor;
        this->Info(CL_DEVICE_VENDOR, vendor);
        return vendor;
    }
    std::string Version() const
    {
        std::string version;
        this->Info(CL_DEVICE_VERSION, version);
        return version;
    }
    std::string Profile() const
    {
        std::string profile;
        this->Info(CL_DEVICE_PROFILE, profile);
        return profile;
    }
    std::string Extensions() const
    {
        std::string extensions;
        this->Info(CL_DEVICE_EXTENSIONS, extensions);
        return extensions;
    }

    bool ImageSupport() const
    {
        bool support;
        this->Info(CL_DEVICE_IMAGE_SUPPORT, support);
        return support;
    }

    operator cl_device_id() const
    {
        return this->id;
    }

    operator bool() const
    {
        return !!this->id;
    }

protected:
    void Info(cl_device_info param, std::string& info) const
    {
        info.clear();

        size_t length;
        cl_int error = clGetDeviceInfo(this->id, param, 0, nullptr, &length);
        if (CL_SUCCESS != error)
        {
            return;
        }

        info.resize(length);
        error = clGetDeviceInfo(this->id, param, length, &info[0], nullptr);
        if (CL_SUCCESS == error)
        {
            info.resize(length - 1);
        }
        else
        {
            info.clear();
        }
    }
    void Info(cl_device_info param, bool& info) const
    {
        cl_bool b;

        cl_int error = clGetDeviceInfo(this->id, param, sizeof(b), &b, nullptr);
        if (CL_SUCCESS != error)
        {
            throw std::runtime_error("Failed to get device info");
        }

        info = b ? true : false;
    }

protected:
    cl_device_id id;
};