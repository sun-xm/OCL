#include "Test.h"
#include <fstream>
#include <random>

using namespace std;

Test::Test()
{
    this->context = CLContext::CreateDefault();
    if (this->context)
    {
        this->queue = this->context.CreateQueue();
    }
}

int Test::ContextCreate()
{
    for (auto& p : CLPlatform::Platforms())
    {
        cout << "Platform: " << p.Name()       << endl;
        cout << "Version:  " << p.Version()    << endl;
        cout << "Vendor:   " << p.Vendor()     << endl;
        cout << "Profile:  " << p.Profile()    << endl;
        cout << "Extends:  " << p.Extensions() << endl;

        for (auto& d : p.Devices())
        {
            cout << "Device:  " << d.Name() << endl;
            cout << "Driver:  " << d.Driver() << endl;
            cout << "Vendor:  " << d.Vendor() << endl;
            cout << "Version: " << d.Version() << endl;
            cout << "Profile: " << d.Profile() << endl;
            cout << "Extends: " << d.Extensions() << endl;
        }
    }

    auto context = CLContext::CreateDefault();
    if (!context)
    {
        return -1;
    }

    return 0;
}

int Test::ContextDevice()
{
    auto context = CLContext::CreateDefault();
    if (!context)
    {
        return -1;
    }

    auto device = context.Device();
    if (!device)
    {
        return -1;
    }

    cout << "Device:  " << device.Name() << endl;
    cout << "Driver:  " << device.Driver() << endl;
    cout << "Vendor:  " << device.Vendor() << endl;
    cout << "Version: " << device.Version() << endl;
    cout << "Profile: " << device.Profile() << endl;
    cout << "Extends: " << device.Extensions() << endl;

    return 0;
}

int Test::BufferMapCopy()
{
    if (!*this)
    {
        return -1;
    }

    const size_t length = 128;

    auto src = this->context.CreateBuffer<int>(CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = this->queue.Map(src, CLFlags::WO);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < src.Length(); i++)
    {
        map[i] = (int)i;
    }
    map.Unmap();

    auto dst = this->context.CreateBuffer<int>(CLFlags::WO, length);
    if (!dst)
    {
        return -1;
    }

    if (!this->queue.Copy(src, dst))
    {
        return -1;
    }

    if (!(map = this->queue.Map(dst, CLFlags::RO)))
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        if (map[i] != (int)i)
        {
            return -1;
        }
    }

    return 0;
}

int Test::BufferReadWrite()
{
    if (!*this)
    {
        return -1;
    }

    const size_t length = 128;

    vector<int> src(length);
    for (size_t i = 0; i < length; i++)
    {
        src[i] = (int)i;
    }

    auto buf = this->context.CreateBuffer<int>(CLFlags::RW, length);
    if (!buf)
    {
        return -1;
    }

    if (!this->queue.Write(buf, src.data()))
    {
        return -1;
    }

    vector<int> dst(length);
    if (!this->queue.Read(buf, &dst[0]))
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        if (src[i] != dst[i])
        {
            return -1;
        }
    }

    return 0;
}

int Test::KernelExecute()
{
    if (!*this || !this->CreateProgram())
    {
        return -1;
    }

    const size_t length = 128;

    auto src = this->context.CreateBuffer<int>(CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = this->queue.Map(src, CLFlags::WO);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        map[i] = (int)i;
    }
    map.Unmap();

    auto dst = this->context.CreateBuffer<int>(CLFlags::WO, length);
    if (!dst)
    {
        return -1;
    }

    auto copy = this->program.CreateKernel("copyIntArray");
    if (!copy)
    {
        return -1;
    }

    copy.Args(src, dst);
    copy.Size({ length });
    if (!this->queue.Execute(copy))
    {
        return -1;
    }

    map = this->queue.Map(dst, CLFlags::RO);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        if (map[i] != (int)i)
        {
            return -1;
        }
    }

    return 0;
}

int Test::KernelBtsort()
{
    if (!*this || !this->CreateProgram())
    {
        return -1;
    }

    const int power = 10;

    auto arr = this->context.CreateBuffer<int>(CLFlags::RW, 1 << power);
    if (!arr)
    {
        return -1;
    }

    auto map = this->queue.Map(arr, CLFlags::WO);
    if (!map)
    {
        return -1;
    }

    default_random_engine e;
    uniform_int_distribution<int> d(0, (int)arr.Length());

    for (size_t i = 0; i < arr.Length(); i++)
    {
        map[i] = d(e);
    }
    map.Unmap();

    auto btsort = this->program.CreateKernel("btsort");
    if (!btsort)
    {
        return -1;
    }
    btsort.Size({ arr.Length() / 2 });

    for (int i = 0; i < power; i++)
    {
        auto tsize = 2 << i;

        for (int j = i; j >= 0; j--)
        {
            if (!btsort.Args(j, tsize, arr) ||
                !this->queue.Execute(btsort))
            {
                return -1;
            }
        }
    }

    map = this->queue.Map(arr, CLFlags::RO);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < arr.Length() - 1; i++)
    {
        if (map[i] > map[i + 1])
        {
            return -1;
        }
    }

    return 0;
}

int Test::EventMapCopy()
{
    if (!*this)
    {
        return -1;
    }

    const size_t length = 128;

    auto src = this->context.CreateBuffer<int>(CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = this->queue.Map(src, CLFlags::WO, {});
    if (!map)
    {
        return -1;
    }

    // Have to wait mapping finish even with write-only flag on Nvidia.
    // Mapped write-only memory is cleared with zeros through mapping. Not understandable.
    map.Wait();

    for (size_t i = 0; i < src.Length(); i++)
    {
        map[i] = (int)i;
    }

    map.Unmap({ map });

    auto dst = this->context.CreateBuffer<int>(CLFlags::WO, length);
    if (!dst)
    {
        return -1;
    }

    if (!this->queue.Copy(src, dst, { map }))
    {
        return -1;
    }

    if (!(map = this->queue.Map(dst, CLFlags::RW, { dst })))
    {
        return -1;
    }
    map.Wait();

    for (size_t i = 0; i < length; i++)
    {
        auto m = map[i];
        if (map[i] != (int)i)
        {
            return -1;
        }
    }

    return 0;
}

int Test::EventReadWrite()
{
    if (!*this)
    {
        return -1;
    }

    const size_t length = 128;

    vector<int> src(length);
    for (size_t i = 0; i < length; i++)
    {
        src[i] = (int)i;
    }

    auto buf = this->context.CreateBuffer<int>(CLFlags::RW, length);
    if (!buf)
    {
        return -1;
    }

    if (!this->queue.Write(buf, src.data(), {}))
    {
        return -1;
    }

    vector<int> dst(length);
    if (!this->queue.Read(buf, &dst[0], { buf }))
    {
        return -1;
    }
    buf.Wait();

    for (size_t i = 0; i < length; i++)
    {
        if (src[i] != dst[i])
        {
            return -1;
        }
    }

    return 0;
}

int Test::EventExecute()
{
    if (!*this || !this->CreateProgram())
    {
        return -1;
    }

    const size_t length = 128;

    auto src = this->context.CreateBuffer<int>(CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = this->queue.Map(src, CLFlags::WO, {});
    if (!map)
    {
        return -1;
    }

    // Have to wait mapping finish even with write-only flag on Nvidia.
    // Mapped write-only memory is cleared with zeros through mapping. Not understandable.
    map.Wait();

    for (size_t i = 0; i < length; i++)
    {
        map[i] = (int)i;
    }
    map.Unmap({ map });

    auto dst = this->context.CreateBuffer<int>(CLFlags::WO, length);
    if (!dst)
    {
        return -1;
    }

    auto copy = this->program.CreateKernel("copyIntArray");
    if (!copy)
    {
        return -1;
    }

    copy.Args(src, dst);
    copy.Size({ length });
    if (!this->queue.Execute(copy, { map }))
    {
        return -1;
    }

    map = this->queue.Map(dst, CLFlags::RO, { copy });
    if (!map)
    {
        return -1;
    }
    map.Wait();

    for (size_t i = 0; i < length; i++)
    {
        if (map[i] != (int)i)
        {
            return -1;
        }
    }

    return 0;
}

int Test::ProgramBinary()
{
    if (!*this || !this->CreateProgram())
    {
        return -1;
    }

    vector<vector<uint8_t>> binaries;
    if (!this->program.GetBinary(binaries))
    {
        return -1;
    }

    ofstream out("binary.bin", ios::binary | ios::trunc);
    if (!this->program.Save(out))
    {
        return -1;
    }
    out.close();

    vector<cl_int> status;
    ifstream in("binary.bin", ios::binary);

    this->program = this->context.LoadProgram(in, &status);
    if (!this->program)
    {
        return -1;
    }

    return 0;
}

bool Test::CreateProgram()
{
    if (this->program)
    {
        return true;
    }

    ifstream file("program.cl");
    if (!file.is_open())
    {
        return false;
    }

    string log;
    this->program = this->context.CreateProgram(string(istreambuf_iterator<char>(file), istreambuf_iterator<char>()).c_str(), "", log);

    return !!this->program;
}