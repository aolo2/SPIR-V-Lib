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

static void
ssa_convert(struct ir *file)
{
    struct uint_vector variables = vector_init();
    
    for (u32 i = 0; i < file->cfg.labels.size; ++i) {
        struct basic_block *block = file->blocks + i;
        struct instruction_list *instruction = block->instructions;
        bool reading = false;
        
        while (instruction) {
            if (instruction->data.opcode == OpVariable) {
                reading = true;
                vector_push(&variables, instruction->data.OpVariable.result_id);
            } else if (reading) {
                // NOTE: we've read all OpVariables. 
                // This is based on the SPIR-V specification!
                break;
            }
            instruction = instruction->next;
        }
    }
    
    struct uint_vector *store_blocks = malloc(variables.size * sizeof(struct uint_vector));
    for (u32 i = 0; i < variables.size; ++i) {
        store_blocks[i] = vector_init();
    }
    
    for (u32 block_index = 0; block_index < file->cfg.labels.size; ++block_index) {
        struct basic_block *block = file->blocks + block_index;
        struct instruction_list *instruction = block->instructions;
        
        while (instruction) {
            if (instruction->data.opcode == OpStore) {
                u32 store_to = search_item_u32(variables.data, variables.size, 
                                               instruction->data.OpStore.pointer);
                vector_push(store_blocks + store_to, block_index);
            }
            instruction = instruction->next;
        }
    }
    
    int a = 1;
    
    // TODO:
    // pass all opstores to compute DF
    // for each node in DF we need to find where control came from
    // WE KNOW that it either directly from one of the opstore nodes or 
    // there is a straight path from opstore node to us (check all :( )
    // we know that because the predecessor is dominated by one of the blocks
    // we need only to know which of the predecessors is dominated by which block
    // do it by checking all :-)
    
    // insert phi functions
    // pseudocode, we have DF and parent_nodes
    // for (node of DF)
    //     phi = {}
    //     for (predecessor of node)
    //         for (parent of parent_nodes)
    //             if (dominates(parent, predecessor)
    //                  phi.add({predecessor, value in parent })
    
    // reindex
    
}