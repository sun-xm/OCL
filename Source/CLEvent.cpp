#include "CLEvent.h"
#include <cassert>
#include <stdexcept>
#include <utility>

using namespace std;

CLEvent::CLEvent(cl_event event) : event(event)
{
}

CLEvent::CLEvent(CLEvent&& other)
{
    *this = move(other);
}

CLEvent::CLEvent(const CLEvent& other) : event(nullptr)
{
    *this = other;
}

CLEvent& CLEvent::operator=(CLEvent&& other)
{
    if (this->event)
    {
        clReleaseEvent(this->event);
    }

    this->event = other.event;
    other.event = nullptr;
    return *this;
}

CLEvent& CLEvent::operator=(const CLEvent& other)
{
    if (other.event && CL_SUCCESS != clRetainEvent(other.event))
    {
        throw runtime_error("Failed to retain event");
    }

    if (this->event)
    {
        clReleaseEvent(this->event);
    }

    this->event = other.event;
    return *this;
}

void CLEvent::Wait() const
{
    if (this->event)
    {
        auto error = clWaitForEvents(1, &this->event);
        assert(CL_SUCCESS == error);
    }
}