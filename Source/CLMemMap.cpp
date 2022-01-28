#include "CLMemMap.h"
#include <cassert>

using namespace std;

CLMemMap::CLMemMap(cl_mem mem, cl_command_queue que, cl_event event, void* map) : mem(nullptr), que(nullptr), map(nullptr)
{
    if (CL_SUCCESS == clRetainMemObject(mem))
    {
        if (CL_SUCCESS == clRetainCommandQueue(que))
        {
            this->map = map;
            this->mem = mem;
            this->que = que;
            this->event = CLEvent(event);
        }
        else
        {
            clReleaseMemObject(mem);
        }
    }
}

CLMemMap::CLMemMap(CLMemMap&& other)
{
    this->map = other.map;
    this->mem = other.mem;
    this->que = other.que;
    this->event = other.event;

    other.map = nullptr;
    other.mem = nullptr;
    other.que = nullptr;
    other.event = CLEvent(nullptr);
}

CLMemMap::~CLMemMap()
{
    this->Unmap({});
}

void CLMemMap::Unmap()
{
    this->Unmap({});
    this->event.Wait();
}

void CLMemMap::Unmap(const initializer_list<CLEvent>& waitList)
{
    if (this->map)
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

    if (this->mem)
    {
        clReleaseMemObject(this->mem);
        this->mem = nullptr;
    }

    if (this->que)
    {
        clReleaseCommandQueue(this->que);
        this->que = nullptr;
    }
}

void* CLMemMap::Get(const initializer_list<CLEvent>& waitList)
{
    if (!this->mem)
    {
        return nullptr;
    }

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
    }

    return this->map;
}