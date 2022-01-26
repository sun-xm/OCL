#pragma once

#include "CLKernel.h"
#include "CLBuffer.h"

class CLQueue
{
private:
    CLQueue(cl_command_queue);
public:
    CLQueue();
    CLQueue(CLQueue&&);
    CLQueue(const CLQueue&);
   ~CLQueue();

    CLQueue& operator=(CLQueue&&);
    CLQueue& operator=(const CLQueue&);

    bool Execute(const CLKernel&);
    bool Execute(const CLKernel&, const std::initializer_list<CLEvent>&);
    bool Map(CLBuffer&);
    bool Map(CLBuffer&, const std::initializer_list<CLEvent>&);
    bool Read(const CLBuffer&, void*, size_t bytes, size_t offset = 0);
    bool Read(const CLBuffer&, void*, size_t bytes, size_t offset, const std::initializer_list<CLEvent>&);
    bool Write(CLBuffer&, void*, size_t bytes, size_t offset = 0);
    bool Write(CLBuffer&, void*, size_t bytes, size_t offset, const std::initializer_list<CLEvent>&);
    void Finish();

    operator cl_command_queue() const
    {
        return this->queue;
    }

    static CLQueue Create(cl_context, cl_device_id device = nullptr);

private:
    cl_command_queue queue;
};