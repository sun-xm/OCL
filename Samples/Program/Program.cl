__kernel
void Copy(__global const uchar* src, __global uchar* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}