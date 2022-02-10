#include "Test.h"
#include <fstream>

using namespace std;

Test::Test()
{
    this->context = CLContext::CreateDefault();
    if (this->context)
    {
        this->queue = this->context.CreateQueue();
    }
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

    if (!(map = this->queue.Map(dst, CLFlags::RO, { dst })))
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