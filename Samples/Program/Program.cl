__kernel
void copy(__global const Float3* src, __global Float3* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}