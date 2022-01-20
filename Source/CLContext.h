#pragma once

#include "CLDevice.h"

class CLContext
{
public:
    CLContext();
   ~CLContext();

    bool Create(cl_device_id);

    operator cl_context() const
    {
        return this->context;
    }

    operator bool() const
    {
        return !!this->context;
    }

private:
    cl_context context;
};