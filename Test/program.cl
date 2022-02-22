void swap(__global int* a, __global int* b)
{
    *a = *a ^ *b;
    *b = *a ^ *b;
    *a = *a ^ *b;
}

__kernel void btsort(uint level, uint tsize, __global int* array)
{
    size_t tid = get_global_id(0);
    size_t lsz = 2 << level;
    size_t lid = tid * 2 / lsz;
    size_t idx = tid + lid * lsz / 2;

    __global int* a = array + idx;
    __global int* b = a + lsz / 2;

    size_t tone = (tid * 2 / tsize) % 2;
    if (( tone && *a < *b) ||
        (!tone && *a > *b))
    {
        swap(a, b);
    }
}

__kernel void copyIntArray(__global const int* src, __global int* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}