#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLQueue.h"
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

    ifstream source("build/Program.cl");
    if (!source.is_open())
    {
        cout << "Failed to open source file" << endl;
        return 0;
    }

    CLProgram program;
    string log;
    if (!program.Create(context, source, "", log))
    {
        cout << log << endl;
        return 0;
    }

    CLKernel kernel = program.CreateKernel("Test");
    if (!kernel)
    {
        cout << "Failed to create kernel" << endl;
        return 0;
    }
    kernel.Size({ 32, 32 });

    CLQueue queue;
    if (!queue.Create(context, device))
    {
        cout << "Failed to create command queue" << endl;
        return 0;
    }
    queue.Enqueue(kernel);

    return 0;
}