__kernel
void copy(__global const uchar* src, __global uchar* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}