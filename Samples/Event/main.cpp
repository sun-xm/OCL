#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLBuffer.h"
#include "CLQueue.h"
#include "Common.h"
#include <cstdint>
#include <fstream>
#include <iostream>

#define SIZE    (128)

#pragma comment(lib, "OpenCL.lib")

using namespace std;

int main(int, char*[])
{
    auto platforms = CLPlatform::Platforms();
    if (platforms.empty())
    {
        cout << "No available opencl platform found" << endl;
        return 0;
    }
    auto platform = platforms[0];

    auto devices = platform.Devices();
    if (devices.empty())
    {
        cout << "No available opencl device found" << endl;
        return 0;
    }
    auto device = devices[0];

    cout << "Using platform: " << platform.Name() << ", device: " << device.Name() << endl;

    CLContext context;
    if (!context.Create(device))
    {
        cout << "Failed to create opencl context" << endl;
        return 0;
    }

    CLQueue queue;
    if (!queue.Create(context, device))
    {
        cout << "Failed to create command queue" << endl;
        return 0;
    }

    ifstream source("build/Event.cl");
    if (!source.is_open())
    {
        cout << "Failed to open source file" << endl;
        return 0;
    }

    string log;
    CLProgram program;
    if (!program.Create(context, source, "", log))
    {
        cout << log << endl;
        return 0;
    }

    CLKernel copy0 = program.CreateKernel("Copy0");
    CLKernel copy1 = program.CreateKernel("Copy1");
    if (!copy0 || !copy1)
    {
        cout << "Failed to create kernel" << endl;
        return 0;
    }

    auto a0 = context.CreateBuffer(CLBuffer::RO, SIZE);
    auto a1 = context.CreateBuffer(CLBuffer::RW, SIZE);
    auto a2 = context.CreateBuffer(CLBuffer::RW, SIZE);
    
    if (!a0 || !a1 || !a2)
    {
        cout << "Failed to create buffers" << endl;
        return 0;
    }

    if (!queue.Map(a0))
    {
        cout << "Failed to map a0" << endl;
        return 0;
    }

    auto mapped = (uint8_t*)a0.Mapped();
    for (uint8_t i = 0; i < SIZE; i++)
    {
        mapped[i] = i;
    }

    copy0.Args(a0, a1);
    copy1.Args(a1, a2);
    copy0.Size({ SIZE });
    copy1.Size({ SIZE });

    if (!queue.Execute(copy0, {}) ||
        !queue.Execute(copy1, { copy0 }))
    {
        cout << "Failed to execute kernel" << endl;
        return 0;
    }

    if (!queue.Map(a2, { copy1 }))
    {
        cout << "Failed to map buffer" << endl;
    }
    a2.Event().Wait();

    mapped = (uint8_t*)a2.Mapped();
    cout << (int)mapped[0] << ' ' << (int)mapped[1] << ' ' << (int)mapped[2] << endl;

    return 0;
}