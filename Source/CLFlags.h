#pragma once

#include <cstdint>

class CLFlags
{
public:
    static const uint32_t RO = 1;
    static const uint32_t WO = 2;
    static const uint32_t RW = RO | WO;
};