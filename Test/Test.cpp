#include "Test.h"
#include <fstream>
#include <random>
#include <mutex>

#define ASSERT(o) if (!o || 0 != o.Error()) return -1

using namespace std;

mutex lock;
void __stdcall callback(cl_event, cl_int status, void* data)
{
    lock_guard<mutex> guard(::lock);

    cout << (char*)data;
    switch (status)
    {
        case CL_COMPLETE:
        {
            cout << " complete";
            break;
        }

        case CL_RUNNING:
        {
            cout << " running";
            break;
        }

        case CL_SUBMITTED:
        {
            cout << " submitted";
            break;
        }

        case CL_QUEUED:
        {
            cout << " queued";
            break;
        }
    }

    cout << endl;
}

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

    cout << "MaxWorkGroupSize: " << device.MaxWorkGroupSize() << endl;
    cout << "MemBaseAddrAlign: " << device.MemBaseAddrAlign() << endl;
    cout << "LocalMemSize:     " << device.LocalMemSize() << endl;

    return 0;
}

int Test::BufferMapCopy()
{
    if (!*this)
    {
        return -1;
    }

    const size_t length = 128;

    auto src = CLBuffer<int>::Create(this->context, CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = src.Map(this->queue, CLFlags::WO);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < src.Length(); i++)
    {
        map[i] = (int)i;
    }
    map.Unmap();

    auto dst = CLBuffer<int>::Create(this->context, CLFlags::WO, length);
    if (!dst)
    {
        return -1;
    }

    if (!dst.Copy(this->queue, src))
    {
        return -1;
    }

    if (!(map = dst.Map(this->queue, CLFlags::RO)))
    {
        return -1;
    }

    for (size_t i = 0; i < dst.Length(); i++)
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

    auto buf = CLBuffer<int>::Create(this->context, CLFlags::RW, length);
    ASSERT(buf);

    if (!buf.Write(this->queue, src.data()))
    {
        return -1;
    }

    src.clear();
    src.resize(length, 123);
    if (!buf.Write(this->queue, src.data()))
    {
        return -1;
    }

    vector<int> dst(length);
    if (!buf.Read(this->queue, &dst[0]))
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

int Test::Buff2DMapCopy()
{
    return 0;
}

int Test::Buff2DReadWrite()
{
    // Correct event completion order should be:
    // write0: complete
    // write1: complete
    // read: complete
    // Unfortunately on Intel platform it not the case. So be careful to use asyn rect read/write method on Intel GPU.
    if (!*this)
    {
        return -1;
    }

    const size_t HW = 4;
    const size_t HH = 4;
    const size_t W  = HW * 2;
    const size_t H  = HH * 2;

    auto buf = CLBuff2D<int>::Create(this->context, CLFlags::RW, W, H);
    ASSERT(buf);

    vector<int> src(W * H, 123);
    if (!buf.Write(this->queue, 0, 0, W, H, src.data(), 0, 0, W * sizeof(src[0]), {}))
    {
        return -1;
    }
    clSetEventCallback(buf.Event(), CL_COMPLETE, callback, "write0: ");

    src = vector<int>(HW * HH, 321);
    if (!buf.Write(this->queue, HW, HH, HW, HH, src.data(), 0, 0, HW * sizeof(src[0]), { buf }))
    {
        return -1;
    }
    clSetEventCallback(buf.Event(), CL_COMPLETE, callback, "write1: ");

    vector<int> dst(W * H, 0);
    if (!buf.Read(this->queue, 0, 0, H, W, &dst[0], 0, 0, W * sizeof(dst[0]), { buf }))
    {
        return -1;
    }
    clSetEventCallback(buf.Event(), CL_COMPLETE, callback, "read: ");

    buf.Wait();

    for (size_t i = 0; i < H; i++)
    {
        for (size_t j = 0; j < W; j++)
        {
            if (dst[i * W + j] != ((i < HH || j < HW) ? 123 : 321))
            {
                return -1;
            }
        }
    }

    return 0;
}

int Test::ImageCreation()
{
    if (!*this)
    {
        return -1;
    }

    auto img = CLImage::Create(this->context, CLFlags::RO, CLImgFmt(CL_RGBA, CL_UNSIGNED_INT8), CLImgDsc(100));
    if (!img || img.Error())
    {
        return -1;
    }

    img = CLImage::Create(this->context, CLFlags::WO, CLImgFmt(CL_R, CL_UNSIGNED_INT16), CLImgDsc(100, 100));
    if (!img || img.Error())
    {
        return -1;
    }

    img = CLImage::Create(this->context, CLFlags::RW, CLImgFmt(CL_RGBA, CL_UNSIGNED_INT8), CLImgDsc(100, 100, 100));
    if (!img || img.Error())
    {
        return -1;
    }

    auto buf = CLBuffer<uint32_t>::Create(this->context, CLFlags::RW, 100);
    img = CLImage::Create(this->context, CLFlags::RW, CLImgFmt(CL_RGBA, CL_UNSIGNED_INT8), CLImgDsc(100, 1, 1, buf));
    if (!img || img.Error())
    {
        return -1;
    }

    return 0;
}

int Test::ImageMapCopy()
{
    if (!*this)
    {
        return -1;
    }

    const size_t w = 100;
    const size_t h = 100;

    auto src = CLImage::Create(this->context, CLFlags::RW, CLImgFmt(CL_RGBA, CL_UNSIGNED_INT8), CLImgDsc(w, h));
    if (!src)
    {
        return -1;
    }

    size_t pitch, slice;
    auto map = src.Map<uint32_t>(this->queue, CLFlags::WO, pitch, slice);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < src.Height(); i++)
    {
        auto ptr = (uint32_t*)((uint8_t*)&map[0] + i * pitch);

        for (size_t j = 0; j < src.Width(); j++)
        {
            ptr[j] = 0x11223344;
        }
    }
    map.Unmap();

    auto dst = CLImage::Create(this->context, CLFlags::RW, src.Format(), CLImgDsc(w / 2, h / 2));
    if (!dst)
    {
        return -1;
    }

    if (!dst.Copy(this->queue, src))
    {
        return -1;
    }

    map = dst.Map<uint32_t>(this->queue, CLFlags::RO, pitch, slice);
    for (size_t i = 0; i < dst.Height(); i++)
    {
        auto ptr = (uint32_t*)((uint8_t*)&map[0] + i * pitch);

        for (size_t j = 0; j < dst.Width(); j++)
        {
            if (0x11223344 != ptr[j])
            {
                return -1;
            }
        }
    }

    return 0;
}

int Test::ImageReadWrite()
{
    if (!*this || !this->CreateProgram())
    {
        return -1;
    }

    const uint32_t w = 100;
    const uint32_t h = 100;

    auto img = CLImage::Create(this->context, CLFlags::RW, CLImgFmt(CL_RGBA, CL_UNSIGNED_INT8), CLImgDsc(w, h));
    ASSERT(img);

    vector<uint32_t> pix(w * h, 0xFFFEFDFC);
    img.Write(this->queue, { 0, 0 }, { w, h }, pix.data(), 0, 0, {});
    if (img.Error())
    {
        return -1;
    }

    // Looks on Intel platform image need to be touched by kernels before can be read out correctly.
    auto touch = this->program.CreateKernel("touchImage");
    touch.Args(img);
    touch.Size({ w, h });
    touch.Execute(this->queue, { img });

    const uint32_t x = 50;
    const uint32_t y = 50;

    pix.clear();
    pix.resize((w - x) * (h - y), 0);

    img.Read(this->queue, { x, y }, { w - x, h - y }, &pix[0], 0, 0, { img });
    if (img.Error())
    {
        return -1;
    }
    img.Wait();

    for (size_t i = 0; i < pix.size(); i++)
    {
        if (0xFFFEFDFC != pix[i])
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

    auto src = CLBuffer<int>::Create(this->context, CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = src.Map(this->queue, CLFlags::WO);
    if (!map)
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        map[i] = (int)i;
    }
    map.Unmap();

    auto dst = CLBuffer<int>::Create(this->context, CLFlags::WO, length);
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
    if (!copy.Execute(this->queue))
    {
        return -1;
    }

    map = dst.Map(this->queue, CLFlags::RO);
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

    auto arr = CLBuffer<int>::Create(this->context, CLFlags::RW, 1 << power);
    if (!arr)
    {
        return -1;
    }

    auto map = arr.Map(this->queue, CLFlags::WO);
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
                !btsort.Execute(this->queue))
            {
                return -1;
            }
        }
    }

    map = arr.Map(this->queue, CLFlags::RO);
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

    auto src = CLBuffer<int>::Create(this->context, CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = src.Map(this->queue, CLFlags::WO, {});
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

    auto dst = CLBuffer<int>::Create(this->context, CLFlags::WO, length);
    if (!dst)
    {
        return -1;
    }

    if (!dst.Copy(this->queue, src, { map }))
    {
        return -1;
    }

    if (!(map = dst.Map(this->queue, CLFlags::RW, { dst })))
    {
        return -1;
    }

    map.Wait();

    for (size_t i = 0; i < dst.Length(); i++)
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

    auto buf = CLBuffer<int>::Create(this->context, CLFlags::RW, length);
    if (!buf)
    {
        return -1;
    }

    if (!buf.Write(this->queue, src.data(), {}))
    {
        return -1;
    }

    vector<int> dst(length);
    if (!buf.Read(this->queue, &dst[0], { buf }))
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

    auto src = CLBuffer<int>::Create(this->context, CLFlags::RO, length);
    if (!src)
    {
        return -1;
    }

    auto map = src.Map(this->queue, CLFlags::WO, {});
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

    auto dst = CLBuffer<int>::Create(this->context, CLFlags::WO, length);
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
    if (!copy.Execute(this->queue, { map }))
    {
        return -1;
    }

    map = dst.Map(this->queue, CLFlags::RO, { copy });
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

    string log;
    vector<cl_int> status;
    ifstream in("binary.bin", ios::binary);

    this->program = this->context.LoadProgram(in, log, &status);
    if (!this->program)
    {
        return -1;
    }

    auto kernel = this->program.CreateKernel("btsort");
    if (!kernel)
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