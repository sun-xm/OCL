#pragma once

#include <string>
#include <CL/cl.h>

class CLDevice
{
public:
    CLDevice(cl_device_id);

    std::string Name() const;
    std::string Driver() const;
    std::string Vendor() const;
    std::string Version() const;
    std::string Profile() const;
    std::string Extensions() const;

    bool ImageSupport() const;

    operator cl_device_id() const
    {
        return this->id;
    }

    operator bool() const
    {
        return !!this->id;
    }

private:
    void Info(cl_device_info, std::string&) const;
    void Info(cl_device_info, bool&) const;

private:
    cl_device_id id;
};