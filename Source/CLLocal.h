#pragma once

template<typename T = char>
struct CLLocal
{
    CLLocal(size_t count) : Size(count * sizeof(T)) {}

    size_t Size;
};