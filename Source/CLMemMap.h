#pragma once

#include "CLEvent.h"
#include <cassert>
#include <initializer_list>

template<typename T>
class CLMemMap
{
public:
    CLMemMap(cl_mem mem, cl_command_queue queue, cl_event event, void* map) : map(nullptr), mem(nullptr), queue(nullptr)
    {
        if (mem && CL_SUCCESS == clRetainMemObject(mem))
        {
            if (queue && CL_SUCCESS == clRetainCommandQueue(queue))
            {
                this->map = map;
                this->mem = mem;
                this->queue = queue;
                this->event = CLEvent(event);
            }
            else
            {
                clReleaseMemObject(mem);
            }
        }
    }
    CLMemMap(CLMemMap&& other)
    {
        this->map = other.map;
        this->mem = other.mem;
        this->queue = other.que;
        this->event = other.event;

        other.map = nullptr;
        other.mem = nullptr;
        other.queue = nullptr;
        other.event = CLEvent(nullptr);
    }
   ~CLMemMap()
    {
        this->Unmap({});
    }

    void Unmap()
    {
        this->Unmap({});
        this->Wait();
    }
    void Unmap(const std::initializer_list<CLEvent>& waits)
    {
        if (this->map)
        {
            std::vector<cl_event> events;
            for (auto& e : waits)
            {
                if (e)
                {
                    events.push_back(e);
                }
            }

            cl_event event;
            auto err = clEnqueueUnmapMemObject(this->queue, this->mem, this->map, (cl_uint)events.size(), events.size() ? events.data() : nullptr, &event);
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

        if (this->queue)
        {
            clReleaseCommandQueue(this->queue);
            this->queue = nullptr;
        }
    }

    void Wait() const
    {
        this->event.Wait();
    }

    T& operator[](size_t index)
    {
        return ((T*)this->map)[index];
    }
    const T& operator[](size_t index) const
    {
        return ((T*)this->map)[index];
    }

    operator bool() const
    {
        return !!this->mem;
    }

private:
    void*  map;
    cl_mem mem;
    cl_command_queue queue;

    CLEvent event;
};