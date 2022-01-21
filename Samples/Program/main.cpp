#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLBuffer.h"
#include "CLQueue.h"
#include "Common.h"
#include <cstdint>
#include <fstream>
#include <iostream>

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

    uint32_t src[128];
    for (uint32_t i = 0; i < (uint32_t)(sizeof(src) / sizeof(src[0])); i++)
    {
        src[i] = i;
    }
    uint32_t dst[sizeof(src) / sizeof(src[0])];

    auto devsrc = context.CreateBuffer(CLBuffer::RO, sizeof(src));
    auto devdst = context.CreateBuffer(CLBuffer::RW, sizeof(dst));

    if (!devsrc || !devdst)
    {
        cout << "Failed to create buffers" << endl;
        return 0;
    }

    queue.Write(devsrc, src, sizeof(src));

    copy.Args((cl_mem)devsrc, (cl_mem)devdst);
    copy.Size({ sizeof(src) / sizeof(src[0]) });
    if (!queue.Execute(copy))
    {
        cout << "Failed to enqueue kernel";
        return 0;
    }
    
    queue.Read(devdst, dst, sizeof(dst));

    return 0;
}