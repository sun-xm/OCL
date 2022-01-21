#pragma once

#include "CLKernel.h"
#include "CLBuffer.h"

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
    bool Map(CLBuffer&, void*, bool blocking = true);
    void Unmap(CLBuffer&);
    bool Read(const CLBuffer&, void*, size_t bytes, size_t offset = 0, bool blocking = true);
    bool Write(CLBuffer&, void*, size_t bytes, size_t offset = 0, bool blocking = true);
    void Finish();

    operator cl_command_queue() const
    {
        return this->queue;
    }

private:
    cl_command_queue queue;
};