#pragma once

#include "CLKernel.h"

class CLQueue
{
public:
    CLQueue();
    CLQueue(CLQueue&&);
    CLQueue(const CLQueue&) = delete;
   ~CLQueue();

    CLQueue& operator=(CLQueue&&);
    CLQueue& operator=(const CLQueue&) = delete;

    bool Create(cl_context, cl_device_id);
    bool Execute(const CLKernel&, bool blocking = true);
    bool Read(cl_mem, void*, size_t bytes);
    bool Read(cl_mem, void*, size_t bytes, size_t offset, bool blocking);
    bool Write(cl_mem, void*, size_t bytes);
    bool Write(cl_mem, void*, size_t bytes, size_t offset, bool blocking);
    void Finish();

    operator cl_command_queue() const
    {
        return this->queue;
    }

private:
    cl_command_queue queue;
};