#pragma once

#include <CL/cl.h>
#include <cassert>
#include <stdexcept>
#include <utility>

class CLEvent
{
public:
    CLEvent() : event(nullptr)
    {
    }
    CLEvent(cl_event event) : CLEvent()
    {
        if (event && CL_SUCCESS == clRetainEvent(event))
        {
            this->event = event;
        }
    }
    CLEvent(CLEvent&& other) : CLEvent()
    {
        *this = std::move(other);
    }
    CLEvent(const CLEvent& other) : CLEvent()
    {
        *this = other;
    }
    virtual ~CLEvent()
    {
       if (this->event)
       {
           clReleaseEvent(this->event);
       }
    }

    CLEvent& operator=(CLEvent&& other)
    {
        cl_event event = this->event;
        this->event = other.event;
        other.event = event;
        return *this;
    }
    CLEvent& operator=(const CLEvent& other)
    {
        if (other.event && CL_SUCCESS != clRetainEvent(other.event))
        {
            throw std::runtime_error("Failed to retain event");
        }

        if (this->event)
        {
            clReleaseEvent(this->event);
        }

        this->event = other.event;
        return *this;
    }

    cl_int Wait() const
    {
        return this->event ? clWaitForEvents(1, &this->event) : 0;
    }

    operator cl_event() const
    {
        return this->event;
    }

    operator bool() const
    {
        return !!this->event;
    }

protected:
    cl_event event;
};