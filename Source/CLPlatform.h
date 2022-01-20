#pragma once

#include <CLDevice.h>
#include <string>
#include <vector>

class CLPlatform
{
public:
    CLPlatform(cl_platform_id);

    std::string Name() const;
    std::string Vendor() const;
    std::string Version() const;
    std::string Profile() const;
    std::string Extensions() const;

    std::vector<CLDevice> Devices() const;

    operator cl_platform_id() const
    {
        return this->id;
    }

    operator bool() const
    {
        return !!this->id;
    }
    
    static std::vector<CLPlatform> Platforms();

private:
    std::string Info(cl_platform_info) const;

private:
    cl_platform_id id;
};