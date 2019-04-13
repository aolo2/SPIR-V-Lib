static struct uint_vector *
ssa_dominance_frontier(struct ir_cfg *cfg, struct cfg_dfs_result *dfs /*, u32 *vertices, u32 n */)
{
    // TODO: this should work for a given set of vertices
    
    struct uint_vector *df = malloc(cfg->labels.size * sizeof(struct uint_vector));
    struct int_stack children_stack = stack_init();
    bool *visited = malloc(cfg->labels.size * sizeof(bool));
    
    for (u32 i = 0; i < cfg->labels.size; ++i) {
        df[i] = vector_init();
    }
    
    for (u32 i = 0; i < dfs->size; ++i) {
        if (cfg->labels.data[i]) {
            u32 vertex = dfs->sorted_postorder[i];
            
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
    }
    
    free(visited);
    stack_free(&children_stack);
    
    // NOTE: iterate the DF
    bool changes = true;
    
    do {
        changes = false;
        
    } while (changes);
    
    return(df);
}