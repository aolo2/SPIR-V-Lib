struct int_stack {
    s32 *data;
    u32 capacity;
    union {
        u32 head;
        u32 size;
    };
};

static struct int_stack
stack_init()
{
    struct int_stack s = {
        .data =  malloc(INITIAL_SIZE * sizeof(s32)),
        .capacity = INITIAL_SIZE,
        .head = 0
    };
    
    return(s);
}

static void
stack_push(struct int_stack *s, s32 x)
{
    if (s->size == s->capacity) {
        s->capacity = (u32) (s->capacity * GROWTH_FACTOR);
        s->data = realloc((void *) s->data, s->capacity * sizeof(s32));
    }
    
    s->data[s->head++] = x;
}

static s32
stack_pop(struct int_stack *s)
{
    ASSERT(s->size);
    return(s->data[--s->head]);
}

static s32
stack_top(struct int_stack *s)
{
    ASSERT(s->size);
    return(s->data[s->head - 1]);
}

static void
stack_clear(struct int_stack *s)
{
    s->size = 0;
}

static void
stack_free(struct int_stack *s)
{
    free(s->data);
}