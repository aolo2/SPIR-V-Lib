static const f32 GROWTH_FACTOR = 1.5f;
static const u32 INITIAL_SIZE  = 10;

struct uint_vector  {
    u32 *data;
    u32 cap;
    union {
        u32 head;
        u32 size;
    };
};

static struct uint_vector
vector_init()
{
    struct uint_vector v = {
        .cap = INITIAL_SIZE,
        .head = 0,
        .data = calloc(INITIAL_SIZE, sizeof(u32)),
    };
    
    return(v);
}

static struct uint_vector
vector_init_sized(u32 size)
{
    struct uint_vector v = {
        .cap = size,
        .head = 0,
        .data = calloc(size, sizeof(u32)),
    };
    
    return(v);
}

static struct uint_vector
vector_init_data(u32 *data, u32 size)
{
    struct uint_vector v = {
        .cap = size,
        .head = size,
        .data = memdup(data, size * sizeof(u32))
    };
    
    return(v);
}

static void
vector_free(struct uint_vector *v)
{
    free(v->data);
    v->cap = 0;
    v->head = 0;
}

static void
vector_push(struct uint_vector *v, u32 item)
{
    if (v->head == v->cap) {
        v->cap = (v->cap > 1 ? (u32) (GROWTH_FACTOR * v->cap) : 5);
        v->data = realloc(v->data, v->cap * sizeof(u32));
    }
    
    v->data[v->head++] = item;
}

// NOTE: push only if not in vector already
static bool
vector_push_maybe(struct uint_vector *v, u32 item)
{
    if (search_item_u32(v->data, v->size, item) == -1) {
        vector_push(v, item);
        return(true);
    }
    return(false);
}

static struct uint_vector
vector_copy(struct uint_vector *v)
{
    struct uint_vector v_new = {
        .cap = v->cap,
        .head = v->head,
        .data = malloc(v->cap * sizeof(u32)),
    };
    
    memcpy(v_new.data, v->data, v->head * sizeof(u32));
    
    return(v_new);
}

static bool
vector_add_all(struct uint_vector *add_to, struct uint_vector *add_me)
{
    bool added = false;
    for (u32 i = 0; i < add_me->size; ++i) {
        bool added_now = vector_push_maybe(add_to, add_me->data[i]);
        added = added || added_now;
    }
    return(added);
}

// NOTE: checks if two vectors WITHOUT DUPLICATES intersect
static bool
vector_unique_same(struct uint_vector *vec1, struct uint_vector *vec2)
{
    if (vec1->size != vec2->size) {
        return(false);
    }
    
    struct uint_vector copy1 = vector_copy(vec1);
    struct uint_vector copy2 = vector_copy(vec2);
    
    qsort(copy1.data, copy1.size, sizeof(u32), compare_u32);
    qsort(copy2.data, copy2.size, sizeof(u32), compare_u32);
    
    bool same = memcmp(copy1.data, copy2.data, copy1.size * sizeof(u32)) == 0;
    
    vector_free(&copy1);
    vector_free(&copy2);
    
    return(same);
}

// NOTE: constructs a union of two vectors WITHOUT DUPLICATES
static struct uint_vector
vector_union(struct uint_vector *vec1, struct uint_vector *vec2)
{
    struct uint_vector copy1 = vector_copy(vec1);
    vector_add_all(&copy1, vec2);
    return(copy1);
}