static void *
memdup(void *src, u32 size)
{
    void *dest = malloc(size);
    return(memcpy(dest, src, size));
}

static inline s32
search_item_u32(u32 *array, u32 count, u32 item)
{
    for (u32 i = 0; i < count; ++i) {
        if (array[i] == item) {
            return(i);
        }
    }
    return(-1);
}
