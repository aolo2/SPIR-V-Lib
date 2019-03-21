struct int_queue {
    struct int_stack s1;
    struct int_stack s2;
    u32 size;
};

static struct int_queue
queue_init()
{
    struct int_queue queue = {
        .s1 = stack_init(),
        .s2 = stack_init()
    };
    
    return(queue);
}

static void
queue_push(struct int_queue *queue, s32 x)
{
    stack_push(&queue->s1, x);
    ++queue->size;
}

static s32
queue_pop(struct int_queue *queue)
{
    ASSERT(queue->size);
    
    if (queue->s2.size == 0) {
        while (queue->s1.size) {
            stack_push(&queue->s2, stack_pop(&queue->s1));
        }
    }
    
    --queue->size;
    
    return(stack_pop(&queue->s2));
}

static void
queue_free(struct int_queue *queue)
{
    stack_free(&queue->s1);
    stack_free(&queue->s2);
}
