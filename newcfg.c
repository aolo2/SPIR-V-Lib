struct edge_list {
    u32 data;
    struct edge_list *next;
};

// NOTE: SPIR-V doesn't demand all result id's to be densely packed,
// so we can remove a res'id in the middle.
struct ir_cfg {
    struct uint_vector labels; // NOTE: zero means 'deleted'
    struct edge_list **out;
    struct edge_list **in;
};

static bool
edge_list_push(struct edge_list **list, u32 item)
{
    struct edge_list *vlist;
    // NOTE: the list is empty
    if (!(*list)) {
        *list = malloc(sizeof(struct edge_list));
        vlist = *list;
        vlist->data = item;
        vlist->next = NULL;
        return(true);
    }
    
    vlist = *list;
    
    while (vlist->data != item && vlist->next) {
        vlist = vlist->next;
    }
    
    if (vlist->data == item) {
        return(false);
    }
    
    vlist->next = malloc(sizeof(struct edge_list));
    vlist->next->data = item;
    vlist->next->next = NULL;
    
    return(true);
}

static bool
edge_list_remove(struct edge_list **list, u32 item)
{
    struct edge_list *vlist = *list;
    
    // NOTE: the list is empty
    if (!vlist) {
        return(false);
    }
    
    // NOTE: remove the first element
    if (vlist->data == item) {
        *list = vlist->next;
        free(vlist);
        return(true);
    }
    
    while (vlist->next && vlist->next->data != item) {
        vlist = vlist->next;
    }
    
    if (vlist->next) {
        struct edge_list *next = vlist->next;
        vlist->next = vlist->next->next;
        free(next);
        return(true);
    }
    
    return(false);
}

static bool
edge_list_replace(struct edge_list *list, u32 old_item, u32 new_item)
{
    while (list && list->data != old_item) {
        list = list->next;
    }
    
    if (list) {
        list->data = new_item;
        return(true);
    }
    
    return(false);
}

static void
edge_list_free(struct edge_list *list)
{
    struct edge_list *next;
    
    while (list) {
        next = list->next;
        free(list);
        list = next;
    }
}

static struct ir_cfg
cfg_init(u32 *labels, u32 nblocks)
{
    struct ir_cfg cfg = {
        .labels = vector_init_data(labels, nblocks),
        .out = calloc(nblocks, sizeof(struct edge_list *)),
        .in = calloc(nblocks, sizeof(struct edge_list *))
    };
    
    return(cfg);
}

static bool
cfg_add_edge(struct ir_cfg *cfg, u32 from, u32 to)
{
    bool added = edge_list_push(cfg->out + from, to);
    added = added && edge_list_push(cfg->in + to, from); // NOTE: returns the same value as the previous call
    return(added);
}

static bool
cfg_remove_edge(struct ir_cfg *cfg, u32 from, u32 to)
{
    bool removed = edge_list_remove(cfg->out + from, to);
    removed = removed && edge_list_remove(cfg->in + to, from);
    return(removed);
}

static bool
cfg_redirect_edge(struct ir_cfg *cfg, u32 from, u32 to_old, u32 to_new)
{
    bool redirected = edge_list_replace(cfg->out[from], to_old, to_new);
    redirected = redirected && edge_list_push(cfg->in + to_new, from);
    redirected = redirected && edge_list_remove(cfg->in + to_old, from);
    return(redirected);
}

static void
cfg_add_vertex(struct ir_cfg *cfg, u32 label)
{
    vector_push(&cfg->labels, label);
    
    cfg->out = realloc(cfg->out, cfg->labels.size * sizeof(struct edge_list *));
    cfg->in = realloc(cfg->in, cfg->labels.size * sizeof(struct edge_list *));
    
    cfg->out[cfg->labels.size - 1] = 0x00;
    cfg->in[cfg->labels.size - 1] = 0x00;
}

static void
cfg_remove_vertex(struct ir_cfg *cfg, u32 index)
{
    struct edge_list *out = cfg->out[index];
    struct edge_list *in = cfg->in[index];
    
    cfg->labels.data[index] = 0;
    
    do {
        edge_list_remove(cfg->in + out->data, index);
        out = out->next;
    } while (out);
    
    do {
        edge_list_remove(cfg->out + in->data, index);
        in = in->next;
    } while (in);
}

static void
cfg_show(struct ir_cfg *cfg) 
{
    for (u32 i = 0; i < cfg->labels.size; ++i) {
        if (cfg->labels.data[i]) {
            printf("Vertex %d\n", cfg->labels.data[i]);
            struct edge_list *edge = cfg->out[i];
            printf("out:");
            while (edge) {
                printf(" []->%d", cfg->labels.data[edge->data]);
                edge = edge->next;
            }
            
            edge = cfg->in[i];
            printf("\nin:");
            while (edge) {
                printf(" %d->[]", cfg->labels.data[edge->data]);
                edge = edge->next;
            }
            printf("\n\n");
        }
    }
}

static void
cfg_free(struct ir_cfg *cfg)
{
    for (u32 i = 0; i < cfg->labels.size; ++i) {
        edge_list_free(cfg->out[i]);
        edge_list_free(cfg->in[i]);
    }
    
    free(cfg->out);
    free(cfg->in);
    
    vector_free(&cfg->labels);
}