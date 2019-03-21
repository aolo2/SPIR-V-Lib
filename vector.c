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
        .data = malloc(INITIAL_SIZE * sizeof(u32)),
    };
    
    return(v);
}

static struct uint_vector
vector_init_sized(u32 size)
{
    struct uint_vector v = {
        .cap = size,
        .head = 0,
        .data = malloc(size * sizeof(u32)),
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