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

    void Wait() const
    {
        if (this->event)
        {
            auto error = clWaitForEvents(1, &this->event);
            assert(CL_SUCCESS == error);
        }
    }

    operator cl_event() const
    {
        return this->event;
    }

    operator bool() const
    {
        return !!this->event;
    }

private:
    cl_event event;
};