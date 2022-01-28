#pragma once

#include "CLEvent.h"
#include <CL/cl.h>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <vector>

class CLMemMap
{
public:
    CLMemMap(cl_mem, cl_command_queue, cl_event, void*);
    CLMemMap(CLMemMap&&);
    CLMemMap(const CLMemMap&) = delete;
   ~CLMemMap();

    CLMemMap& operator=(const CLMemMap&) = delete;
    CLMemMap& operator=(CLMemMap&&) = delete;

    void Unmap();
    void Unmap(const std::initializer_list<CLEvent>&);

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
        return !!this->mem;
    }

    template<typename T>
    T* Get(const std::initializer_list<CLEvent>& waitList = {})
    {
        return (T*)this->Get(waitList);
    }

    template<typename T>
    const T* Get(const std::initializer_list<CLEvent>& waitList = {}) const
    {
        return (T*)this->Get(waitList);
    }

private:
    void* Get(const std::initializer_list<CLEvent>&);

private:
    void*  map;
    cl_mem mem;
    cl_command_queue que;
    
    mutable CLEvent event;
};