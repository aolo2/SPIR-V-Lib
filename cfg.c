struct cfgc {
    u32 nver;
    u32 neds;
    struct uint_vector edges_from;
    struct uint_vector in_edges_from;
    struct uint_vector edges;
    struct uint_vector in_edges;
};

struct dfs_result {
    u32 *preorder;
    u32 *parent;
    u32 *sorted;
};

static struct dfs_result
cfgc_dfs(struct cfgc *graph)
{
    struct dfs_result res = {
        .preorder = calloc(graph->nver, sizeof(u32)),
        .sorted   = malloc(graph->nver * sizeof(u32)),
        .parent   = malloc(graph->nver * sizeof(u32))
    };
    
    struct int_stack node_stack = stack_init();
    stack_push(&node_stack, 0);
    
    u32 t1 = 0;
    
    while (node_stack.size) {
        u32 node_index = stack_pop(&node_stack); 
        //printf("[DEBUG] Visited node %d\n", node_index);
        
        u32 edges_from = graph->edges_from.data[node_index];
        u32 edges_to   = graph->edges_from.data[node_index + 1];
        
        res.sorted[t1] = node_index;
        res.preorder[node_index] = t1++;
        
        for (u32 i = edges_from; i < edges_to; ++i) {
            u32 child = graph->edges.data[i];
            if (!res.preorder[child]) {
                res.parent[child] = node_index;
                stack_push(&node_stack, child);
            }
        }
    }
    
    // NOTE: will not get read, init just for completeness sake
    res.parent[0] = 0;
    
    stack_free(&node_stack);
    
    return(res);
}

static struct uint_vector 
cfgc_bfs_(struct cfgc *graph, u32 root, s32 terminate)
{
    bool *visited = calloc(graph->nver, sizeof(bool));
    
    struct uint_vector sorted = vector_init();
    struct int_queue node_queue = queue_init();
    
    queue_push(&node_queue, root);
    
    while (node_queue.size) {
        u32 node_index = queue_pop(&node_queue); 
        if (visited[node_index] || (terminate != -1 && node_index == (u32) terminate)) {
            continue;
        }
        
        //printf("[DEBUG] Visited node %d\n", node_index);
        
        u32 edges_from = graph->edges_from.data[node_index];
        u32 edges_to = graph->edges_from.data[node_index + 1];
        
        vector_push(&sorted, node_index);
        visited[node_index] = true;
        
        for (u32 i = edges_from; i < edges_to; ++i) {
            u32 child = graph->edges.data[i];
            if (!visited[child] && (terminate != -1 && child != (u32) terminate)) {
                queue_push(&node_queue, child);
            }
        }
    }
    
    queue_free(&node_queue);
    free(visited);
    
    return(sorted);
}

#if 0
static struct bfs_result
cfg_bfs(struct cfgc *graph)
{
    return(cfgc_bfs_(graph, 0, -1, NULL, NULL, NULL));
}
#endif

static struct uint_vector
cfgc_bfs_restricted(struct cfgc *graph, u32 header, u32 merge)
{
    return(cfgc_bfs_(graph, header, (s32) merge));
}

static void
dfs_result_free(struct dfs_result *res)
{
    free(res->preorder);
    free(res->sorted);
    free(res->parent);
}

static bool
cfgc_preorder_less(u32 *preorder, u32 a, u32 b)
{
    return(preorder[a] < preorder[b]);
}

static u32
cfgc_find_min(u32 *preorder, u32 *sdom, u32 *label, s32 *ancestor, u32 v)
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
        if (cfgc_preorder_less(preorder, sdom[label[ancestor[v]]], sdom[label[v]])) {
            label[v] = label[ancestor[v]];
        }
        ancestor[v] = ancestor[u];
    }
    
    stack_free(&s);
    
    return(label[v]);
}

// NOTE: dominators as per Lengauer-Tarjan, -1 means N/A
// preorder contains T1 for each vertex, parent has DFS parents
static s32 *
cfgc_dominators(struct cfgc *input)
{
    struct dfs_result dfs = cfgc_dfs(input);
    struct int_stack *bucket = malloc(input->nver * sizeof(struct int_stack));
    
    u32 *sdom = malloc(input->nver * sizeof(u32));
    u32 *label = malloc(input->nver * sizeof(u32));
    s32 *ancestor = malloc(input->nver * sizeof(s32));
    
    s32 *dom = malloc(input->nver * sizeof(s32));
    
    for (u32 i = 0; i < input->nver; ++i) {
        bucket[i] = stack_init();
        sdom[i] = label[i] = i;
        ancestor[i] = -1;
        dom[i] = -1;
    }
    
    // NOTE: for all vertices in reverse preorder except ROOT! Root always 
    // has preorder 0, so we can just skip the first (last) element
    for (u32 i = 0; i < input->nver - 1; ++i) {
        u32 w = dfs.sorted[input->nver - 1 - i];
        u32 in_edges_from = input->in_edges_from.data[w];
        u32 in_edges_to = (w == input->nver - 1 ? input->neds : input->in_edges_from.data[w + 1]);
        
        // NOTE: for each incident edge
        for (u32 j = in_edges_from; j < in_edges_to; ++j) {
            u32 v = input->in_edges.data[j];
            u32 u = cfgc_find_min(dfs.preorder, sdom, label, ancestor, v);
            if (cfgc_preorder_less(dfs.preorder, sdom[u], sdom[w])) {
                sdom[w] = sdom[u];
            }
        }
        
        ancestor[w] = dfs.parent[w];
        stack_push(bucket + sdom[w], w);
        
        struct int_stack parent_bucket = bucket[dfs.parent[w]];
        for (u32 j = 0; j < parent_bucket.size; ++j) {
            u32 v = parent_bucket.data[j];
            u32 u = cfgc_find_min(dfs.preorder, sdom, label, ancestor, v);
            
            dom[v] = (sdom[u] == sdom[v] ? dfs.parent[w] : u);
        }
        
        stack_clear(bucket + dfs.parent[w]);
    }
    
    // NOTE: for all vertices except ROOT in preorder
    for (u32 i = 1; i < input->nver; ++i) {
        u32 w = dfs.sorted[i];
        if (dom[w] != (s32) sdom[w]) {
            dom[w] = dom[dom[w]];
        }
    }
    
    dom[0] = -1;
    
    for (u32 i = 0; i < input->nver; ++i) {
        stack_free(bucket + i);
    }
    
    free(bucket);
    free(sdom);
    free(label);
    free(ancestor);
    
    dfs_result_free(&dfs);
    
    return(dom);
}

static void
cfgc_free(struct cfgc *graph)
{
    vector_free(&graph->edges_from);
    vector_free(&graph->in_edges_from);
    vector_free(&graph->in_edges);
    vector_free(&graph->edges);
}