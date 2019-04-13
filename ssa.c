static struct uint_vector *
ssa_dominance_frontier_all(struct ir_cfg *cfg, struct cfg_dfs_result *dfs)
{
    struct uint_vector *df = malloc(cfg->labels.size * sizeof(struct uint_vector));
    struct int_stack children_stack = stack_init();
    bool *visited = malloc(cfg->labels.size * sizeof(bool));
    
    for (u32 i = 0; i < cfg->labels.size; ++i) {
        df[i] = vector_init();
    }
    
    for (u32 i = 0; i < dfs->size; ++i) {
        u32 vertex = dfs->sorted_postorder[i];
        ASSERT(cfg->labels.data[i]);
        
        // NOTE: only compute DF for passed vertices
        memset(visited, 0x00, cfg->labels.size * sizeof(bool));
        stack_clear(&children_stack);
        
        // NOTE: DF-local
        struct edge_list *edge = cfg->out[vertex];
        while (edge) {
            u32 to = edge->data;
            stack_push(&children_stack, to); // NOTE: used in DF-up some five lines below
            if (cfg->dominators[to] != (s32) vertex) {
                vector_push(df + vertex, to);
            }
            edge = edge->next;
        }
        
        visited[vertex] = true;
        
        // NOTE: DF-up
        while (children_stack.size > 0) {
            u32 node = stack_pop(&children_stack);
            if (visited[node]) {
                continue;
            }
            
            for (u32 j = 0; j < df[node].size; ++j) {
                u32 dfv = df[node].data[j];
                if (cfg->dominators[dfv] != (s32) vertex) {
                    vector_push_maybe(df + vertex, dfv);
                }
            }
            
            struct edge_list *edge = cfg->out[node];
            while (edge) {
                if (!visited[edge->data]) {
                    stack_push(&children_stack, edge->data);
                }
                edge = edge->next;
            }
            
            visited[node] = true;
        }
    }
    
    free(visited);
    stack_free(&children_stack);
    
    return(df);
}

static struct uint_vector
ssa_dominance_frontier_subset(struct ir_cfg *cfg, struct cfg_dfs_result *dfs, struct uint_vector *vertices)
{
    struct uint_vector *df = ssa_dominance_frontier_all(cfg, dfs);
    struct uint_vector total_df = vector_init();
    
    for (u32 i = 0; i < vertices->size; ++i) {
        u32 vertex = vertices->data[i];
        for (u32 j = 0; j < df[vertex].size; ++j) {
            vector_push_maybe(&total_df, df[vertex].data[j]); 
        }
    }
    
    return(total_df);
}

static struct uint_vector
ssa_iterate_frontier(struct ir_cfg *cfg, struct cfg_dfs_result *dfs, struct uint_vector *df,
                     struct uint_vector *vertices)
{
    
    struct uint_vector next_df;
    while (true) {
        // TODO: clear temp memory
        struct uint_vector arg = vector_union(df, vertices);
        next_df = ssa_dominance_frontier_subset(cfg, dfs, &arg);
        
        if (vector_unique_same(df, &next_df)) {
            break;
        } else {
            vector_free(df);
            df = &next_df;
        }
    };
    
    return(next_df);
}


static struct uint_vector
ssa_dominance_frontier(struct ir_cfg *cfg, struct cfg_dfs_result *dfs, u32 *vertices, u32 n)
{
    struct uint_vector vertices_vec = vector_init_data(vertices, n);
    struct uint_vector df1 = ssa_dominance_frontier_subset(cfg, dfs, &vertices_vec);
    struct uint_vector dfp = ssa_iterate_frontier(cfg, dfs, &df1, &vertices_vec);
    
    return(dfp);
}