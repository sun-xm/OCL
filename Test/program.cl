__kernel void copyIntArray(__global const int* src, __global int* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}