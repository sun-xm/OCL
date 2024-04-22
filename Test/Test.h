#pragma once

#include "CLContext.h"
#include "CLFlags.h"

class Test
{
public:
    Test();

    int ContextCreate();
    int ContextDevice();
    int Buffer2D3DCopy();
    int BufferMapCopy();
    int BufferReadWrite();
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