#pragma once

template<typename T>
struct CLLocal
{
    CLLocal(size_t count) : Size(count * sizeof<T>) {}

    size_t Size;
};