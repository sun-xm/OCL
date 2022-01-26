#pragma once

#include "CLBuffer.h"
#include "CLDevice.h"
#include "CLQueue.h"

class CLContext
{
private:
    CLContext(cl_context);
public:
    CLContext(CLContext&&);
    CLContext(const CLContext&);
   ~CLContext();

    CLContext& operator=(CLContext&&);
    CLContext& operator=(const CLContext&);

    CLQueue  CreateQueue();
    CLBuffer CreateBuffer(uint64_t flags, size_t bytes);

    operator cl_context() const
    {
        return this->context;
    }

    operator bool() const
    {
        return !!this->context;
    }

    static CLContext Create(cl_device_id);

private:
    cl_context context;
};