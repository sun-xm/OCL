#include "CLDevice.h"
#include <exception>

using namespace std;

CLDevice::CLDevice(cl_device_id id) : id(id)
{
}

string CLDevice::Name() const
{
    string name;
    this->Info(CL_DEVICE_NAME, name);
    return name;
}

string CLDevice::Driver() const
{
    string driver;
    this->Info(CL_DRIVER_VERSION, driver);
    return driver;
}

string CLDevice::Vendor() const
{
    string vendor;
    this->Info(CL_DEVICE_VENDOR, vendor);
    return vendor;
}

string CLDevice::Version() const
{
    string version;
    this->Info(CL_DEVICE_VERSION, version);
    return version;
}

string CLDevice::Profile() const
{
    string profile;
    this->Info(CL_DEVICE_PROFILE, profile);
    return profile;
}

string CLDevice::Extensions() const
{
    string extensions;
    this->Info(CL_DEVICE_EXTENSIONS, extensions);
    return extensions;
}

bool CLDevice::ImageSupport() const
{
    bool support;
    this->Info(CL_DEVICE_IMAGE_SUPPORT, support);
    return support;
}

void CLDevice::Info(cl_device_info param, string& info) const
{
    info.clear();

    size_t length;
    if (CL_SUCCESS != clGetDeviceInfo(this->id, param, 0, nullptr, &length))
    {
        return;
    }

    info.resize(length);

    if (CL_SUCCESS == clGetDeviceInfo(this->id, param, length, &info[0], nullptr))
    {
        info.resize(length - 1);
    }
    else
    {
        info.clear();
    }
}

void CLDevice::Info(cl_device_info param, bool& info) const
{
    cl_bool b;
    if (CL_SUCCESS != clGetDeviceInfo(this->id, param, sizeof(b), &b, nullptr))
    {
        throw runtime_error("Failed to get device info");
    }

    info = b ? true : false;
}