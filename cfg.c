enum mark {
    WHITE,
    GRAY,
    BLACK
};

struct edge_list {
    u32 data;
    struct edge_list *next;
};

struct cfg_dfs_result {
    u32 *preorder;
    u32 *postorder;
    u32 *parent;
    u32 *sorted_postorder;
    u32 *sorted_preorder;
    u32 size;
};

// NOTE: SPIR-V doesn't demand all result id's to be densely packed,
// so we can remove a res'id in the middle.
struct ir_cfg {
    struct uint_vector labels; // NOTE: zero means 'deleted'
    u32 *conditions;
    s32 *dominators;
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
        .conditions = calloc(nblocks, sizeof(u32)),
        .out = calloc(nblocks, sizeof(struct edge_list *)),
        .in = calloc(nblocks, sizeof(struct edge_list *))
    };
    
    return(cfg);
}

bool
cfg_add_edge(struct ir_cfg *cfg, u32 from, u32 to)
{
    bool added = edge_list_push(cfg->out + from, to);
    added = added && edge_list_push(cfg->in + to, from); // NOTE: returns the same value as the previous call
    return(added);
}

bool
cfg_remove_edge(struct ir_cfg *cfg, u32 from, u32 to)
{
    bool removed = edge_list_remove(cfg->out + from, to);
    removed = removed && edge_list_remove(cfg->in + to, from);
    return(removed);
}

bool
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

void
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

void
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

struct cfg_dfs_result
cfg_dfs(struct ir_cfg *cfg)
{
    u32 maxnver = cfg->labels.size;
    struct cfg_dfs_result dfs = {
        .preorder         = calloc(maxnver,  sizeof(u32)),
        .postorder        = calloc(maxnver,  sizeof(u32)),
        .parent           = malloc(maxnver * sizeof(u32)),
        .sorted_preorder  = malloc(maxnver * sizeof(u32)),
        .sorted_postorder = malloc(maxnver * sizeof(u32))
    };
    
    enum mark *marks = malloc(maxnver * sizeof(enum mark));
    memset(marks, WHITE, maxnver * sizeof(enum mark));
    
    u32 t1 = 0;
    u32 t2 = 0;
    struct int_stack node_stack = stack_init();
    stack_push(&node_stack, 0);
    
    while (node_stack.size) {
        u32 node = stack_pop(&node_stack);
        
        if (marks[node] == WHITE) {
            
            // NOTE: pre-visit actions
            marks[node] = GRAY;
            dfs.sorted_preorder[t1] = node;
            dfs.preorder[node] = t1++;
            // ======================
            
            stack_push(&node_stack, node);
            
            struct edge_list *edge = cfg->out[node];
            while (edge) {
                u32 child = edge->data;
                if (marks[child] == WHITE) {
                    dfs.parent[child] = node;
                    stack_push(&node_stack, child);
                }
                edge = edge->next;
            }
        } else if (marks[node] == GRAY) {
            // NOTE: post-visit actions
            marks[node] = BLACK;
            dfs.sorted_postorder[t2] = node;
            dfs.postorder[node] = t2++;
            // =======================
        }
    }
    
    ASSERT(t1 == t2);
    
    dfs.size = t1;
    dfs.parent[0] = UINT32_MAX;
    
    return(dfs);
}

static struct uint_vector
cfg_bfs_order_(struct ir_cfg *cfg, u32 root, s32 terminate)
{
    struct uint_vector order = vector_init();
    struct int_queue node_queue = queue_init();
    
    queue_push(&node_queue, root);
    
    bool *visited = calloc(cfg->labels.size, sizeof(bool));
    visited[root] = true;
    
    while (node_queue.size) {
        u32 node = queue_pop(&node_queue);
        struct edge_list *edge = cfg->out[node];
        
        vector_push(&order, node);
        
        while (edge) {
            u32 child = edge->data;
            if (!visited[child] && (terminate == -1 || child != (u32) terminate)) {
                queue_push(&node_queue, child);
                visited[child] = true;
            }
            edge = edge->next;
        }
    }
    
    free(visited);
    
    return(order);
}

struct uint_vector
cfg_bfs_order(struct ir_cfg *cfg)
{
    return(cfg_bfs_order_(cfg, 0, -1));
}

struct uint_vector
cfg_bfs_order_r(struct ir_cfg *cfg, u32 root, s32 terminate)
{
    return(cfg_bfs_order_(cfg, root, terminate));
}


static inline bool
cfg_preorder_less(u32 *preorder, u32 a, u32 b)
{
    return(preorder[a] < preorder[b]);
}

static u32
cfg_find_min(u32 *preorder, u32 *sdom, u32 *label, s32 *ancestor, u32 v)
{
    if (ancestor[v] == -1) {
        return(v);
    }
    
    struct int_stack s = stack_init();
    u32 u = v;
    
    while (ancestor[ancestor[u]] != -1) {
        stack_push(&s, u);
        u = ancestor[u];
    }
    
    while (s.size) {
        v = stack_pop(&s);
        if (cfg_preorder_less(preorder, sdom[label[ancestor[v]]], sdom[label[v]])) {
            label[v] = label[ancestor[v]];
        }
        ancestor[v] = ancestor[u];
    }
    
    stack_free(&s);
    
    return(label[v]);
}

// NOTE: dominators as per Lengauer-Tarjan, -1 means N/A
s32 *
cfg_dominators(struct ir_cfg *input, struct cfg_dfs_result *dfs)
{
    u32 nver = input->labels.size;
    struct int_stack *bucket = malloc(nver * sizeof(struct int_stack));
    
    u32 *sdom = malloc(nver * sizeof(u32));
    u32 *label = malloc(nver * sizeof(u32));
    s32 *ancestor = malloc(nver * sizeof(s32));
    
    s32 *dom = malloc(nver * sizeof(s32));
    
    for (u32 i = 0; i < nver; ++i) {
        bucket[i] = stack_init();
        sdom[i] = label[i] = i;
        ancestor[i] = -1;
        dom[i] = -1;
    }
    
    // NOTE: for all vertices in reverse preorder except ROOT! Root always 
    // has preorder 0, so we can just skip the first (last) element
    for (u32 i = 0; i < nver - 1; ++i) {
        u32 w = dfs->sorted_preorder[nver - 1 - i];
        
        // NOTE: for each incident edge
        struct edge_list *in_edge = input->in[w];
        while (in_edge) {
            u32 u = cfg_find_min(dfs->preorder, sdom, label, ancestor, in_edge->data);
            if (cfg_preorder_less(dfs->preorder, sdom[u], sdom[w])) {
                sdom[w] = sdom[u];
            }
            in_edge = in_edge->next;
        }
        
        ancestor[w] = dfs->parent[w];
        stack_push(bucket + sdom[w], w);
        
        struct int_stack parent_bucket = bucket[dfs->parent[w]];
        for (u32 j = 0; j < parent_bucket.size; ++j) {
            u32 v = parent_bucket.data[j];
            u32 u = cfg_find_min(dfs->preorder, sdom, label, ancestor, v);
            
            dom[v] = (sdom[u] == sdom[v] ? dfs->parent[w] : u);
        }
        
        stack_clear(bucket + dfs->parent[w]);
    }
    
    // NOTE: for all vertices except ROOT in preorder
    for (u32 i = 1; i < nver; ++i) {
        u32 w = dfs->sorted_preorder[i];
        if (dom[w] != (s32) sdom[w]) {
            dom[w] = dom[dom[w]];
        }
    }
    
    dom[0] = -1;
    
    for (u32 i = 0; i < nver; ++i) {
        stack_free(bucket + i);
    }
    
    free(bucket);
    free(sdom);
    free(label);
    free(ancestor);
    
    //dfs_result_free(&dfs);
    
    return(dom);
}

u32
cfg_whichpred(struct ir_cfg *cfg, u32 block_index, u32 pred_index)
{
    struct edge_list *edge = cfg->in[block_index];
    u32 which = 0;
    
    while (edge) {
        if (edge->data == pred_index) {
            break;
        }
        ++which;
        edge = edge->next;
    }
    
    ASSERT(edge != NULL);
    
    return(which);
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
    free(cfg->conditions);
    free(cfg->dominators);
}