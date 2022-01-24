#pragma once

#include <cstdint>
#include <CL/cl.h>

class CLBuffer
{
private:
    CLBuffer(cl_mem);
public:
    CLBuffer(CLBuffer&&);
    CLBuffer(const CLBuffer&);
   ~CLBuffer();

    CLBuffer& operator=(CLBuffer&&);
    CLBuffer& operator=(const CLBuffer&);

    bool Map(cl_command_queue, void*, bool blocking = true);
    void Unmap();
    
    void* Mapped() const
    {
        return this->map;
    }

    operator cl_mem() const
    {
        return this->mem;
    }
    
    operator bool() const
    {
        return !!this->mem;
    }

    static CLBuffer Create(cl_context, uint64_t flags, size_t bytes);

    static uint64_t RO;
    static uint64_t WO;
    static uint64_t RW;

private:
    cl_mem mem;
    void*  map;
    cl_command_queue que;
};