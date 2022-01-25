#pragma once

#include <CL/cl.h>

class CLEvent
{
public:
    CLEvent(cl_event = nullptr);
    CLEvent(CLEvent&&);
    CLEvent(const CLEvent&);

    CLEvent& operator=(CLEvent&&);
    CLEvent& operator=(const CLEvent&);

    void Wait() const;

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