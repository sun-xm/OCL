#pragma once

#include "CLBuffer.h"
#include "CLDevice.h"
#include "CLProgram.h"
#include "CLQueue.h"
#include <iostream>
#include <string>

class CLContext
{
private:
    CLContext(cl_context);
public:
    CLContext();
    CLContext(CLContext&&);
    CLContext(const CLContext&);
   ~CLContext();

    CLContext& operator=(CLContext&&);
    CLContext& operator=(const CLContext&);

    CLProgram CreateProgram(const char* source, const char* options, std::string& log);
    CLProgram CreateProgram(std::istream& source, const char* options, std::string& log);
    CLQueue   CreateQueue();

    template<typename T>
    CLBuffer<T> CreateBuffer(uint32_t flags, size_t length)
    {
        return CLBuffer<T>::Create(this->context, flags, length);
    }

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