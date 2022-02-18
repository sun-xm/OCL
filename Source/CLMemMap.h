#pragma once

#include "CLEvent.h"
#include <cassert>

template<typename T>
class CLMemMap
{
public:
    CLMemMap() : map(nullptr), mem(nullptr), queue(nullptr)
    {
    }
    CLMemMap(cl_mem mem, cl_command_queue queue, cl_event event, void* map) : CLMemMap()
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
    CLMemMap(CLMemMap&& other) : CLMemMap()
    {
        *this = std::move(other);
    }
    CLMemMap(const CLMemMap&) = delete;
   ~CLMemMap()
    {
        this->Unmap({});
    }

    CLMemMap& operator=(CLMemMap&& other)
    {
        auto map = this->map;
        auto mem = this->mem;
        auto que = this->queue;
        auto evt = this->event;

        this->map = other.map;
        this->mem = other.mem;
        this->queue = other.queue;
        this->event = other.event;

        other.map = map;
        other.mem = mem;
        other.queue = que;
        other.event = evt;

        return *this;
    }
    CLMemMap& operator=(const CLMemMap&) = delete;

    void Unmap()
    {
        this->Unmap({});
        this->Wait();
    }
    void Unmap(const std::vector<cl_event>& waits)
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

    CLEvent Event() const
    {
        return this->event;
    }
    operator cl_event() const
    {
        return (cl_event)this->event;
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

    mutable CLEvent event;
};