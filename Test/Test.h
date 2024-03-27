#pragma once

#include "CLContext.h"
#include "CLFlags.h"

class Test
{
public:
    Test();

    int ContextCreate();
    int ContextDevice();
    int BufferMapCopy();
    int BufferReadWrite();
    int Buff2DMapCopy();
    int Buff2DReadWrite();
    int ImageCreation();
    int ImageMapCopy();
    int ImageReadWrite();
    int KernelExecute();
    int KernelBtsort();
    int EventMapCopy();
    int EventReadWrite();
    int EventExecute();
    int ProgramBinary();

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