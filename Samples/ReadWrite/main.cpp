#pragma comment(lib, "OpenCL.lib")

#include "CLContext.h"
#include "CLFlags.h"
#include "CLPlatform.h"
#include <iostream>

using namespace std;

int main(int, char*[])
{
    auto context = CLContext::CreateDefault();
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

    vector<uint32_t> src(128);
    for (size_t i = 0; i < src.size(); i++)
    {
        src[i] = (uint32_t)i;
    }

    auto buf = context.CreateBuffer<uint32_t>(CLFlags::RW, src.size());
    if (!queue.Write(buf, src.data()))
    {
        cout << "Failed to write buffer" << endl;
        return 0;
    }

    vector<uint32_t> dst(buf.Length());
    if (!queue.Read(buf, &dst[0]))
    {
        cout << "Failed to read buffer" << endl;
        return 0;
    }

    cout << dst[0] << ' ' << dst[1] << ' ' << dst[dst.size() - 1] << endl;

    return 0;
}