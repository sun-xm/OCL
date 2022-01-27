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
    typedef std::function<CLEvent(cl_mem, cl_command_queue, const std::initializer_list<CLEvent>&, std::vector<uint8_t>&)> SyncFunc;
    typedef std::function<CLEvent(cl_mem, cl_command_queue, const std::initializer_list<CLEvent>&, const std::vector<uint8_t>&)> FlushFunc;

public:
    CLMemMap(cl_mem, cl_command_queue, cl_event, void*, const SyncFunc&, const FlushFunc&, size_t);
    CLMemMap(CLMemMap&&);
    CLMemMap(const CLMemMap&) = delete;
   ~CLMemMap();

    CLMemMap& operator=(const CLMemMap&) = delete;
    CLMemMap& operator=(CLMemMap&&) = delete;

    void Flush();
    void Flush(const std::initializer_list<CLEvent>&);
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
        return (T*)this->Get(false, waitList);
    }

    template<typename T>
    const T* Get(const std::initializer_list<CLEvent>& waitList = {}) const
    {
        return (T*)this->Get(false, waitList);
    }

    template<typename T>
    T* GetSynced(const std::initializer_list<CLEvent>& waitList = {})
    {
        return (T*)this->Get(true, waitList);
    }

    template<typename T>
    const T* GetSynced(const std::initializer_list<CLEvent>& waitList = {}) const
    {
        return (T*)this->Get(true, waitList);
    }

private:
    void* Get(bool sync, const std::initializer_list<CLEvent>&);

private:
    void*  map;
    cl_mem mem;
    cl_command_queue que;

    SyncFunc sync;
    FlushFunc flush;
    std::vector<uint8_t> data;
    
    mutable CLEvent event;
};