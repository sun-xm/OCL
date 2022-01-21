__kernel
void Copy(__global const uint* src, __global uint* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}