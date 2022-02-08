#pragma comment(lib, "OpenCL.lib")

#include "CLPlatform.h"
#include "CLContext.h"
#include "CLCommon.h"
#include "CLFlags.h"
#include "Float3.cl"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

#define SIZE    (128)

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

    ostringstream source;

    ifstream file;
    if (!(file = ifstream("build/Float3.cl")).is_open())
    {
        cout << "Failed to open Float3.cl" << endl;
        return 0;
    }
    source << string(istreambuf_iterator<char>(file), istreambuf_iterator<char>()) << endl;

    if (!(file = ifstream("build/Program.cl")).is_open())
    {
        cout << "Failed to open build/Program.cl" << endl;
        return 0;
    }
    source << string(istreambuf_iterator<char>(file), istreambuf_iterator<char>()) << endl;

    string log;
    CLProgram program = context.CreateProgram(source.str().c_str(), "", log);
    if (!program)
    {
        cout << log << endl;
        return 0;
    }

    CLKernel copy = program.CreateKernel("copy");
    if (!copy)
    {
        cout << "Failed to create kernel" << endl;
        return 0;
    }

    auto src = context.CreateBuffer<Float3>(CLFlags::RO, SIZE);
    auto dst = context.CreateBuffer<Float3>(CLFlags::RW, src.Length());

    if (!src || !dst)
    {
        cout << "Failed to create buffer" << endl;
        return 0;
    }

    auto maps = queue.Map(src, CLFlags::WO);
    if (!maps)
    {
        cout << "Failed to map buffer" << endl;
        return 0;
    }

    auto length = src.Length();
    for (size_t i = 0; i < length; i++)
    {
        maps[i] = { (float)i, (float)i, (float)i };
    }
    maps.Unmap();

    copy.Args(src, dst);
    copy.Size({ src.Length() });
    if (!queue.Execute(copy))
    {
        cout << "Failed to enqueue kernel" << endl;
        return 0;
    }

    auto mapd = queue.Map(dst, CLFlags::RO);
    if (!mapd)
    {
        cout << "Failed to map buffer" << endl;
        return 0;
    }

    auto f3 = mapd[0];
    cout << f3.X << ' ' << f3.Y << ' ' << f3.Z << endl;

    f3 = mapd[dst.Length() - 1];
    cout << f3.X << ' ' << f3.Y << ' ' << f3.Z << endl;

    return 0;
}