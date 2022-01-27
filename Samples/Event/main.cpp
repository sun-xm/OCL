#pragma comment(lib, "OpenCL.lib")

#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLBuffer.h"
#include "CLQueue.h"
#include "CLCommon.h"
#include <cstdint>
#include <fstream>
#include <iostream>

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

    CLKernel init = program.CreateKernel("Init");
    CLKernel copy = program.CreateKernel("Copy");
    if (!init || !copy)
    {
        cout << "Failed to create kernel" << endl;
        return 0;
    }

    auto src = context.CreateBuffer(CLBuffer::RW, 128 * sizeof(uint32_t));
    auto dst = context.CreateBuffer(CLBuffer::RO, src.Length());

    if (!src || !dst)
    {
        cout << "Failed to create buffer" << endl;
        return 0;
    }

    init.Args(src);
    init.Size({ src.Length() / sizeof(uint32_t) });

    if (!queue.Execute(init, {}))
    {
        cout << "Failed to execute init" << endl;
        return 0;
    }

    copy.Args(src, dst);
    copy.Size( { src.Length() / sizeof(uint32_t) });

    if (!queue.Execute(copy, { init }))
    {
        cout << "Failed to execute copy" << endl;
        return 0;
    }

    auto map = queue.Map(dst, { copy });
    if (!map)
    {
        cout << "Failed to map buffer" << endl;
        return 0;
    }
    map.Event().Wait();

    auto mapped = map.Get<uint32_t>();
    cout << mapped[0] << ' ' << mapped[1] << ' ' << mapped[2] << endl;

    return 0;
}