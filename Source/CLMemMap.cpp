#include "CLMemMap.h"
#include <cassert>

using namespace std;

CLMemMap::CLMemMap(cl_mem mem, cl_command_queue que, cl_event event, void* map, const SyncFunc& sync, const FlushFunc& flush, size_t size)
  : mem(nullptr), que(nullptr), map(nullptr)
{
    if (CL_SUCCESS == clRetainMemObject(mem))
    {
        if (CL_SUCCESS == clRetainCommandQueue(que))
        {
            this->mem = mem;
            this->que = que;
            this->map = map;
            this->sync = sync;
            this->flush = flush;
            this->event = CLEvent(event);
            this->data.resize(size);
        }
        else
        {
            clReleaseMemObject(mem);
        }
    }
}

CLMemMap::~CLMemMap()
{
    this->Unmap({});
    
    if (this->mem)
    {
        clReleaseMemObject(this->mem);
    }

    if (this->que)
    {
        clReleaseCommandQueue(this->que);
    }
}

void CLMemMap::Flush()
{
    this->Flush({});
    this->event.Wait();
}

void CLMemMap::Flush(const std::initializer_list<CLEvent>& waitList)
{
    if (this->flush)
    {
        this->event = this->flush(this->mem, this->que, waitList, this->data);
    }
}

void CLMemMap::Unmap()
{
    this->Unmap({});
}

void CLMemMap::Unmap(const initializer_list<CLEvent>& waitList)
{
    if (this->mem && this->map)
    {
        vector<cl_event> events;
        for (auto& e : waitList)
        {
            if (e)
            {
                events.push_back(e);
            }
        }

        cl_event event;
        auto err = clEnqueueUnmapMemObject(this->que, this->mem, this->map, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
        assert(CL_SUCCESS == err);
        this->map = nullptr;

        this->event = CLEvent(event);
        clReleaseEvent(event);
    }
}

void* CLMemMap::Get(bool sync, const initializer_list<CLEvent>& waitList)
{
    if (this->map)
    {
        if (waitList.size())
        {
            vector<cl_event> events;
            for (auto& e : waitList)
            {
                if (e)
                {
                    events.push_back(e);
                }
            }

            if (events.size())
            {
                clWaitForEvents((cl_uint)events.size(), events.data());
            }
        }
        return this->map;
    }

    if (sync && this->sync)
    {
        this->event = this->sync(this->mem, this->que, waitList, this->data);
        this->event.Wait();
    }
    
    return &this->data[0];
}