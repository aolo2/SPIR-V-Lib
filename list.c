struct list {
    struct list *next;
    u32 key;
    char *val;
};

const u32 LIST_ITEM_SIZE = sizeof(struct list);

static struct list*
list_make_item(u32 key, char *val, u32 width)
{
    struct list *l = malloc(LIST_ITEM_SIZE);
    
    l->next = NULL;
    l->key = key;
    l->val = malloc(width);
    
    memcpy(l->val, val, width);
    
    return(l);
}

// NOTE: returns address of newly appended item
static struct list*
list_add_after(struct list *add_after_me, struct list *add_me)
{
    ASSERT(add_after_me->next == NULL);
    add_after_me->next = add_me;
    return(add_me);
}

static bool
list_add_to_end(struct list *head, struct list *add_me)
{
    // NOTE: check first item
    if (head->key == add_me->key) { 
        return(false); 
    }
    
    while (head->next) {
        if (head->key == add_me->key) { 
            return(false); 
        }
        head = head->next;
    }
    
    list_add_after(head, add_me);
    
    return(true);
}

static void
list_del_after(struct list *del_after_me)
{
    struct list *to_del = del_after_me->next;
    ASSERT(to_del);
    
    del_after_me->next = to_del->next;
    free(to_del);
}

static void
list_free_item(struct list *item)
{
    free(item->val);
    free(item);
}

static void
list_del_head(struct list **data, u32 index)
{
    struct list *next = data[index]->next;
    list_free_item(data[index]);
    data[index] = next;
}

static void
list_free(struct list *head)
{
    struct list *next;
    
    do {
        next = head->next;
        list_free_item(head);
        head = next;
    } while (head);
}