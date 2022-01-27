#include "CLMemMap.h"
#include <cassert>
#include <vector>

using namespace std;

CLMemMap::CLMemMap(cl_mem mem, cl_command_queue que, cl_event event, void* map, size_t size) : mem(nullptr), que(nullptr), map(nullptr)
{
    if (CL_SUCCESS == clRetainMemObject(mem))
    {
        if (CL_SUCCESS == clRetainCommandQueue(que))
        {
            this->mem = mem;
            this->que = que;
            this->map = map;
            this->event = CLEvent(event);

            if (!map && size)
            {
            }
        }
        else
        {
            clReleaseMemObject(mem);
        }
    }
}

CLMemMap::~CLMemMap()
{
    this->Unmap();
    
    if (this->mem)
    {
        clReleaseMemObject(this->mem);
    }

    if (this->que)
    {
        clReleaseCommandQueue(this->que);
    }
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