const u32 BUCKET_COUNT = 1000;
const u32 MURMUR_SEED = 0x2BF9D07B;

struct hashset {
    u32 buckets;
    u32 width;
    struct list **data;
};

static u32
murmur3_32(const char  *key, u64 len) {
    u32 h = MURMUR_SEED;
    
    if (len > 3) {
        const u32* key_x4 = (const u32*) key;
        size_t i = len >> 2;
        
        do {
            u32 k = *key_x4++;
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h = (h * 5) + 0xe6546b64;
        } while (--i);
        
        key = (const char*) key_x4;
    }
    
    
    if (len & 3) {
        size_t i = len & 3;
        u32 k = 0;
        key = &key[i - 1];
        
        do {
            k <<= 8;
            k |= *key--;
        } while (--i);
        
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
    }
    
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    
    return(h);
}

static struct hashset
set_init(u32 width)
{
    struct hashset h = {
        .buckets = BUCKET_COUNT,
        .data = calloc(BUCKET_COUNT, sizeof(struct list *)),
        .width = width
    };
    
    return(h);
}

static void
set_free(struct hashset *set)
{
    for (u32 i = 0; i < set->buckets; ++i) {
        if (set->data[i]) {
            list_free(set->data[i]);
        }
    }
    
    free(set->data);
}

static bool
set_add(struct hashset *set, char *item)
{
    u32 hash = murmur3_32(item, set->width);
    u32 index = hash % set->buckets;
    
    struct list *bucket = set->data[index];
    struct list *to_add = list_make_item(hash, item, set->width);
    bool added = false;
    
    if (bucket) {
        added = list_add_to_end(bucket, to_add); 
    } else {
        set->data[index] = to_add;
        added = true;
    }
    
    if (!added) {
        list_free_item(to_add);
    }
    
    return(added);
}

static struct list*
set_seek(struct hashset *set, u32 key)
{
    u32 index = key % set->buckets;
    struct list *bucket = set->data[index];
    
    if (bucket) {
        do {
            if (bucket->key == key) { 
                return(bucket); 
            } 
            bucket = bucket->next;
        } while (bucket);
    } else {
        return(NULL);
    }
    
    return(NULL);
}

static bool
set_del(struct hashset *set, u32 key)
{
    u32 index = key % set->buckets;
    
    struct list *bucket = set->data[index];
    struct list *last = bucket;
    
    if (bucket) {
        do {
            if (bucket->key == key) {
                if (last == set->data[index]) { 
                    list_del_head(set->data, index); 
                } else {
                    list_del_after(last); 
                }
                return(true);
            }
            
            last = bucket;
            bucket = bucket->next;
        } while (bucket);
    } else { 
        return(false); 
    }
    
    return(false);
}

static bool
set_has(struct hashset *set, void *item)
{
    u32 key = murmur3_32(item, set->width);
    return(set_seek(set, key) != NULL);
}