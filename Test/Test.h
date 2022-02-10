#pragma once

#include "CLContext.h"
#include "CLFlags.h"

class Test
{
public:
    Test();

    int BufferMapCopy();
    int BufferReadWrite();
    int KernelExecute();
    int EventMapCopy();
    int EventReadWrite();
    int EventExecute();

    operator bool() const
    {
        return this->context && this->queue;
    }

private:
    bool CreateProgram();

private:
    CLContext context;
    CLProgram program;
    CLQueue   queue;
};