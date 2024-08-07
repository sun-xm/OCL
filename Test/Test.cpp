#include "Test.h"
#include <CLBuffer.h>
#include <CLImage.h>
#include <CLKernel.h>
#include <fstream>
#include <random>
#include <mutex>

#define ASSERT(o) if (!o || 0 != o.Error()) return -1
#define DIVUP(a, b) ((a + b - 1) / b)
#define PITCH(var)  (cl_uint)(var.Pitch())
#define SLICE(var)  (cl_uint)(var.Slice())

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
        this->queue = CLQueue::Create(this->context);
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

int Test::Buffer2D3DCopy()
{
    if (!*this)
    {
        return -1;
    }

    const size_t width  = 5;
    const size_t height = 5;
    const size_t depth  = 5;

    // copy 2d to 2d
    auto s2d = CLBuff2D<int>::Create(this->context, CLFlags::RO, width, height);
    auto d2d = CLBuff2D<int>::Create(this->context, CLFlags::WO, width - 1, height - 1);
    ASSERT(s2d);
    ASSERT(d2d);

    {
        auto map = s2d.Map(this->queue, CLFlags::WO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < s2d.Height(); i++)
        {
            auto row = (int*)((char*)&map[0] + i * s2d.Pitch());
            for (size_t j = 0; j < s2d.Width(); j++)
            {
                row[j] = 123;
            }
        }
    }

    vector<int> view(width * height);
    s2d.Read(this->queue, &view[0]);

    if (d2d.Copy(this->queue, s2d, 0, 0, width, height, 0, 0))
    {
        return -1;
    }

    if (d2d.Copy(this->queue, s2d))
    {
        auto map = d2d.Map(this->queue, CLFlags::RO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < d2d.Height(); i++)
        {
            auto row = (int*)((char*)&map[0] + i * d2d.Pitch());
            for (size_t j = 0; j < d2d.Width(); j++)
            {
                if (123 != row[j])
                {
                    return -1;
                }
            }
        }
    }
    else
    {
        return -1;
    }

    // Copy 3d to 3d
    auto s3d = CLBuff3D<int>::Create(this->context, CLFlags::RO, width, height, depth);
    auto d3d = CLBuff3D<int>::Create(this->context, CLFlags::WO, width + 1, height + 1, depth + 1);
    ASSERT(s3d);
    ASSERT(d3d);

    {
        auto map = s3d.Map(this->queue, CLFlags::WO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < s3d.Depth(); i++)
        {
            auto slc = (char*)&map[0] + i * s3d.Slice();
            for (size_t j = 0; j < s3d.Height(); j++)
            {
                auto row = (int*)(slc + j * s3d.Pitch());
                for (size_t k = 0; k < s3d.Width(); k++)
                {
                    row[k] = 123;
                }
            }
        }
    }

    if (d3d.Copy(this->queue, s3d))
    {
        auto map = d3d.Map(this->queue, CLFlags::RO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < d3d.Depth(); i++)
        {
            auto slc = (char*)&map[0] + i * d3d.Slice();
            for (size_t j = 0; j < d3d.Height(); j++)
            {
                auto row = (int*)(slc + j * d3d.Pitch());
                for (size_t k = 0; k < d3d.Width(); k++)
                {
                    if (i < depth && j < height && k < width)
                    {
                        if (123 != row[k])
                        {
                            return -1;
                        }
                    }
                }
            }
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

int Test::BufferMapCopy()
{
    if (!*this)
    {
        return -1;
    }

    const size_t width  = 5;
    const size_t height = 5;
    const size_t depth  = 4;
    const size_t length = width * height * depth;

    // copy 1d to 1d
    auto src = CLBuffer<int>::Create(this->context, CLFlags::RO, length);
    ASSERT(src);
    {
        auto map = src.Map(this->queue, CLFlags::WO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < src.Length(); i++)
        {
            map[i] = (int)i;
        }
    }

    auto dst = CLBuffer<int>::Create(this->context, CLFlags::WO, length);
    ASSERT(dst);

    if (dst.Copy(this->queue, src))
    {
        auto map = src.Map(this->queue, CLFlags::WO, length / 4, length / 2);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < length / 2; i++)
        {
            map[i] = 555;
        }
    }
    else
    {
        return -1;
    }

    if (dst.Copy(this->queue, src, length / 4, length / 2, length / 4))
    {
        auto ms = src.Map(this->queue, CLFlags::RO);
        auto md = dst.Map(this->queue, CLFlags::RO);
        if (!ms || !md)
        {
            return -1;
        }

        for (size_t i = 0; i < length; i++)
        {
            if (ms[i] != md[i])
            {
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }

    // copy 2d to 1d
    auto b2d = CLBuff2D<int>::Create(this->context, CLFlags::RO, width, height * depth);
    ASSERT(b2d);
    {
        auto map = b2d.Map(this->queue, CLFlags::WO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < height * depth; i++)
        {
            auto row = (int*)((char*)&map[0] + i * b2d.Pitch());
            for (size_t j = 0; j < width; j++)
            {
                row[j] = 222;
            }
        }
    }

    if(dst.Copy(this->queue, b2d))
    {
        auto map = dst.Map(this->queue, CLFlags::RO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < length; i++)
        {
            if (222 != map[i])
            {
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }

    // copy 3d to 1d
    auto b3d = CLBuff3D<int>::Create(this->context, CLFlags::RO, width, height, depth);
    ASSERT(b3d);
    {
        auto map = b3d.Map(this->queue, CLFlags::WO);
        for (size_t i = 0; i < b3d.Depth(); i++)
        {
            auto slc = (char*)&map[0] + i * b3d.Slice();

            for (size_t j = 0; j < b3d.Height(); j++)
            {
                auto row = (int*)(slc + j * b3d.Pitch());

                for (size_t k = 0; k < b3d.Width(); k++)
                {
                    row[k] = 333;
                }
            }
        }
    }

    if (dst.Copy(this->queue, b3d))
    {
        auto map = dst.Map(this->queue, CLFlags::RO);
        if (!map)
        {
            return -1;
        }

        for (size_t i = 0; i < length; i++)
        {
            if (333 != map[i])
            {
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }

    // Copy 1d to 2d
    b2d = CLBuff2D<int>::Create(this->context, CLFlags::RO, width * depth, height);
    ASSERT(b2d);

    if (b2d.Copy(this->queue, dst))
    {
        auto map = b2d.Map(this->queue, CLFlags::RO);
        for (size_t i = 0; i < b2d.Height(); i++)
        {
            auto row = (int*)((char*)&map[0] + i * b2d.Pitch());
            for (size_t j = 0; j < b2d.Width(); j++)
            {
                if (333 != row[j])
                {
                    return false;
                }
            }
        }
    }
    else
    {
        return -1;
    }

    // Copy 1d to 3d
    b3d = CLBuff3D<int>::Create(this->context, CLFlags::RO, width, height, depth);
    ASSERT(b3d);

    if (b3d.Copy(this->queue, dst))
    {
        auto map = b3d.Map(this->queue, CLFlags::RO);
        for (size_t i = 0; i < b3d.Depth(); i++)
        {
            auto slc = (char*)&map[0] + i * b3d.Slice();

            for (size_t j = 0; j < b3d.Height(); j++)
            {
                auto row = (int*)(slc + j * b3d.Pitch());

                for (size_t k = 0; k < b3d.Width(); k++)
                {
                    if (333 != row[k])
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

int Test::BufferReadWrite()
{
    if (!*this)
    {
        return -1;
    }

    const size_t width  = 5;
    const size_t height = 5;
    const size_t depth  = 4;
    const size_t length = width * height * depth;

    auto buf = CLBuffer<int>::Create(this->context, CLFlags::RW, length);
    ASSERT(buf);

    vector<int> src(length, 123);
    if (!buf.Write(this->queue, src.data()))
    {
        return -1;
    }

    vector<int> dst(length, 0);
    if (!buf.Read(this->queue, &dst[0]))
    {
        return -1;
    }

    for (size_t i = 0; i < dst.size(); i++)
    {
        if (123 != dst[i])
        {
            return -1;
        }
    }

    auto b2d = CLBuff2D<int>::Create(this->context, CLFlags::RW, width, height * depth);
    ASSERT(b2d);

    if (!b2d.Write(this->queue, src.data()))
    {
        return -1;
    }

    dst = vector<int>(length, 0);
    if (!b2d.Read(this->queue, &dst[0]))
    {
        return -1;
    }

    for (size_t i = 0; i < dst.size(); i++)
    {
        if (123 != dst[i])
        {
            return -1;
        }
    }

    auto b3d = CLBuff3D<int>::Create(this->context, CLFlags::RW, width, height, depth);
    ASSERT(b3d);

    if (!b3d.Write(this->queue, src.data()))
    {
        return -1;
    }

    dst = vector<int>(length, 0);
    if (!b3d.Read(this->queue, &dst[0]))
    {
        return -1;
    }

    for (size_t i = 0; i < dst.size(); i++)
    {
        if (123 != dst[i])
        {
            return -1;
        }
    }

    return 0;
}

int Test::BufferAsyncWrite()
{
    const size_t width  = 100;
    const size_t height = 100;

    vector<int> src0(width * height, 123);
    vector<int> src1(width * height / 4, 321);

    auto dst = CLBuff2D<int>::Create(this->context, CLFlags::RO, width, height);
    ASSERT(dst);

    if (!dst.Write(this->queue, src0.data(), {}))
    {
        return -1;
    }

    if (!dst.Write(this->queue, width / 2, height / 2, width / 2, height / 2, src1.data(), 0, 0, 0, { dst }))
    {
        return -1;
    }

    if (!dst.Read(this->queue, &src0[0], { dst }))
    {
        return -1;
    }
    dst.Wait();

    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            if (i >= height / 2 && j >= width / 2)
            {
                if (321 != src0[i * width + j])
                {
                    return -1;
                }
            }
            else if (123 != src0[i * width + j])
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
    if (!img.Write(this->queue, { 0, 0 }, { w, h }, pix.data(), 0, 0, {}))
    {
        return -1;
    }

    // Looks on Intel platform image need to be touched by kernels before can be read out correctly.
    // auto touch = this->program.CreateKernel("touchImage");
    // touch.Args(img);
    // touch.Size({ w, h });
    // touch.Execute(this->queue, { img });

    const uint32_t x = 50;
    const uint32_t y = 50;

    pix.clear();
    pix.resize((w - x) * (h - y), 0);

    if (!img.Read(this->queue, { x, y }, { w - x, h - y }, &pix[0], 0, 0, { img }))
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

    auto copy = CLKernel::Create(this->program, "copyIntArray");
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

    auto btsort = CLKernel::Create(this->program, "btsort");
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

int Test::KernelSumup()
{
    if (!*this || !this->CreateProgram())
    {
        return -1;
    }

    auto sumup = CLKernel::Create(this->program, "sumup");
    ASSERT(sumup);

    const size_t width  = 50;
    const size_t height = 50;

    auto ones = CLBuff2D<int>::Create(this->context, CLFlags::RO, width, height);
    if (!ones.Write(this->queue, vector<int>(ones.Width() * ones.Height(), 1).data()))
    {
        return -1;
    }

    const size_t cols = 8;
    const size_t rows = 8;
    size_t xgroups = DIVUP(ones.Width(),  cols);
    size_t ygroups = DIVUP(ones.Height(), rows);
    auto sumups = CLBuffer<int>::Create(this->context, CLFlags::WO, xgroups * ygroups);

    sumup.Args(ones, PITCH(ones), (cl_uint)ones.Width(), (cl_uint)ones.Height(), sumups, CLLocal<int>(cols * rows));
    sumup.Size({ xgroups * cols, ygroups * rows }, { cols, rows });
    if (!sumup.Execute(this->queue))
    {
        return -1;
    }

    auto map = sumups.Map(this->queue, CLFlags::RO);

    int sum = 0;
    for (size_t i = 0; i < sumups.Length(); i++)
    {
        sum += map[i];
    }

    if (width * height != sum)
    {
        return -1;
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

    auto copy = CLKernel::Create(this->program, "copyIntArray");
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

    this->program = CLProgram::Load(this->context, in, log, &status);
    if (!this->program)
    {
        return -1;
    }

    auto kernel = CLKernel::Create(this->program, "btsort");
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
    this->program = CLProgram::Create(this->context, string(istreambuf_iterator<char>(file), istreambuf_iterator<char>()).c_str(), "", log);

    return !!this->program;
}