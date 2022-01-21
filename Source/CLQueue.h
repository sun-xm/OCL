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
    bool Enqueue(const CLKernel&);

private:
    cl_command_queue queue;
};