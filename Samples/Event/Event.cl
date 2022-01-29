__kernel
void init(__global uint* arr)
{
    size_t idx = get_global_id(0);
    arr[idx] = (uint)idx;
}

__kernel
void copy(__global const uint* src, __global uint* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}