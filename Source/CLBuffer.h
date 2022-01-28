#pragma once

#include "CLMemMap.h"
#include <cstdint>
#include <initializer_list>

class CLBuffer
{
private:
    CLBuffer(cl_mem);
public:
    CLBuffer();
    CLBuffer(CLBuffer&&);
    CLBuffer(const CLBuffer&);
   ~CLBuffer();

    CLBuffer& operator=(CLBuffer&&);
    CLBuffer& operator=(const CLBuffer&);

    CLEvent& Event() const
    {
        return this->event;
    }
    operator CLEvent&() const
    {
        return this->event;
    }

    CLMemMap Map(cl_command_queue, uint32_t flags);
    CLMemMap Map(cl_command_queue, uint32_t flags, const std::initializer_list<CLEvent>& waitList);

    size_t Size() const;

    operator cl_mem() const
    {
        return this->mem;
    }
    
    operator bool() const
    {
        return !!this->mem;
    }

    static CLBuffer Create(cl_context, uint32_t flags, size_t bytes);

private:
    cl_mem mem;
    mutable CLEvent event;
};