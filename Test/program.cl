#define PITCH(var) pitch__##var
#define SLICE(var) slice__##var
#define ROW2D(type, pointer, y)         ((__global type*)((__global char*)(pointer) + (y) * PITCH(pointer)))
#define ROW3D(type, pointer, y, z)      ((__global type*)((__global char*)(pointer) + (y) * PITCH(pointer) + (z) * SLICE(pointer)))
#define ELM2D(type, pointer, x, y)      (ROW2D(type, pointer, y)[x])
#define ELM3D(type, pointer, x, y, z)   (ROW3D(type, pointer, y, z)[x])

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

__kernel void sumup(global const int* values,
                                 uint PITCH(values),
                                 uint width,
                                 uint height,
                    global       int* sumups,
                    local        int* reduce)
{
    size_t x = get_global_id(0);
    size_t y = get_global_id(1);

    size_t lid = get_local_id(1)  * get_local_size(0)  + get_local_id(0);
    reduce[lid] = (x < width && y < height) ? ELM2D(int, values, x, y) : 0;
    barrier(CLK_LOCAL_MEM_FENCE);

    size_t lsz = get_local_size(0) * get_local_size(1);
    while (lsz > 1)
    {
        lsz = lsz >> 1;

        if (lid < lsz)
        {
            reduce[lid] += reduce[lid + lsz];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (0 == lid)
    {
        size_t gid = get_group_id(1) * get_num_groups(0) + get_group_id(0);
        sumups[gid] = reduce[0];
    }
}

__kernel void copyIntArray(__global const int* src, __global int* dst)
{
    size_t idx = get_global_id(0);
    dst[idx] = src[idx];
}

__kernel void touchImage(__read_only image2d_t img)
{
    size_t x = get_global_id(0);
    size_t y = get_global_id(1);

    uint4 pixel = read_imageui(img, CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST, (int2)(x, y));
    if (0 == x && 0 == y)
    {
        printf("image touched\n");
    }
}