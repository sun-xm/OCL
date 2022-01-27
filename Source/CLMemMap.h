#pragma once

#include "CLEvent.h"
#include <CL/cl.h>
#include <initializer_list>

class CLMemMap
{
public:
    CLMemMap(cl_mem, cl_command_queue, cl_event, void*, size_t);
    CLMemMap(const CLMemMap&) = delete;
    CLMemMap(CLMemMap&&) = delete;
   ~CLMemMap();

    CLMemMap& operator=(const CLMemMap&) = delete;
    CLMemMap& operator=(CLMemMap&&) = delete;

    void Unmap(const std::initializer_list<CLEvent>& waitList = {});

    const CLEvent& Event() const
    {
        return this->event;
    }
    operator const CLEvent() const
    {
        return this->event;
    }

    operator bool() const
    {
        return !!this->map;
    }

    template<typename T>
    T* Get()
    {
        return (T*)this->map;
    }

    template<typename T>
    const T* Get() const
    {
        return (T*)this->map;
    }

private:
    void*  map;
    cl_mem mem;
    cl_command_queue que;

    CLEvent event;
};