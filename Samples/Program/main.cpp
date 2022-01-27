#pragma comment(lib, "OpenCL.lib")

#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLBuffer.h"
#include "CLQueue.h"
#include "CLCommon.h"
#include <cstdint>
#include <fstream>
#include <iostream>

#define SIZE    (128)

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

    auto context = CLContext::Create(device);
    if (!context)
    {
        cout << "Failed to create opencl context" << endl;
        return 0;
    }

    auto queue = context.CreateQueue();
    if (!queue)
    {
        cout << "Failed to create command queue" << endl;
        return 0;
    }

    ifstream source("build/Program.cl");
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

    CLKernel copy = program.CreateKernel("Copy");
    if (!copy)
    {
        cout << "Failed to create kernel" << endl;
        return 0;
    }

    auto src = context.CreateBuffer(CLBuffer::RO, SIZE);
    auto dst = context.CreateBuffer(CLBuffer::RW, src.Size());

    if (!src || !dst)
    {
        cout << "Failed to create buffer" << endl;
        return 0;
    }

    auto maps = queue.Map(src);
    if (!maps)
    {
        cout << "Failed to map buffer" << endl;
        return 0;
    }

    auto mapped = maps.Get<uint8_t>();
    auto size = src.Size();
    for (size_t i = 0; i < size; i++)
    {
        mapped[i] = (uint8_t)i;
    }
    maps.Flush();

    copy.Args(src, dst);
    copy.Size({ src.Size() });
    if (!queue.Execute(copy))
    {
        cout << "Failed to enqueue kernel" << endl;
        return 0;
    }

    auto mapd = queue.Map(dst);
    if (!mapd)
    {
        cout << "Failed to map buffer" << endl;
        return 0;
    }

    mapped = mapd.GetSynced<uint8_t>();
    cout << (int)mapped[0] << ' ' << (int)mapped[1] << ' ' << (int)mapped[2] << endl;

    return 0;
}