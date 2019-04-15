static bool
dominates(u32 parent, s32 child, s32 *dominators)
{
    if (parent == (u32) child) {
        return(true);
    }
    
    do {
        child = dominators[child];
        if (child == (s32) parent) {
            return(true);
        }
    } while (child != -1);
    
    return(false);
}

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
ssa_dominance_frontier(struct ir_cfg *cfg, struct cfg_dfs_result *dfs, struct uint_vector *vertices)
{
    struct uint_vector df1 = ssa_dominance_frontier_subset(cfg, dfs, vertices);
    struct uint_vector dfp = ssa_iterate_frontier(cfg, dfs, &df1, vertices);
    return(dfp);
}

// NOTE: variable is passed by value, we modify it here
static u32
ssa_insert_variable(struct ir *file, u32 block_index, struct instruction_t variable, u32 version)
{
    u32 res_id = file->header.bound;
    file->header.bound++;
    
    variable.OpVariable.result_id = res_id;
    prepend_instruction(file->blocks + block_index, variable);
    
    return(res_id);
}

static void
ssa_traverse(struct ir *file, struct int_stack *versions, u32 *counter, struct instruction_t original_variable, u32 mapping[100], u32 block_index)
{
    u32 variable = original_variable.OpVariable.result_id;
    struct instruction_list *inst = file->blocks[block_index].instructions;
    
    while (inst) {
        if (inst->data.opcode == OpLoad) {
            u32 pointer = inst->data.OpLoad.pointer;
            if (pointer == variable) {
                u32 version = stack_top(versions);
                u32 version_id = mapping[version];
                inst->data.OpLoad.pointer = version_id;
            }
        }
        
        if (inst->data.opcode == OpStore && inst->data.OpStore.pointer == variable) {
            ssa_insert_variable(file, 0 /* TODO: get actual root block */, original_variable, *counter);
            stack_push(versions, *counter);
            *counter += 1;
        }
        
        inst = inst->next;
    }
    
    struct edge_list *succ_edge = file->cfg.out[block_index];
    u32 succ_order = 0;
    while (succ_edge) {
        int succ_index = succ_edge->data;
        u32 pred_index = cfg_whichpred(&file->cfg, succ_index, block_index);
        struct basic_block *succ = file->blocks + succ_index;
        struct instruction_list *succ_inst = succ->instructions;
        
        while (succ_inst) {
            if (succ_inst->data.opcode == OpPhi) {
                succ_inst->data.OpPhi.variables[pred_index] = mapping[stack_top(versions)];
            }
            succ_inst = succ_inst->next;
        }
        
        ++succ_order;
        succ_edge = succ_edge->next;
    }
    
    for (u32 i = 0; i < file->cfg.labels.size; ++i) {
        if (file->cfg.dominators[i] == block_index) {
            ssa_traverse(file, versions, counter, original_variable, mapping, i);
        }
    }
    
    inst = file->blocks[block_index].instructions;
    while (inst) {
        if (inst->data.opcode == OpStore) {
            if (search_item_u32(mapping, 100, inst->data.OpStore.pointer) != -1) { 
                stack_pop(versions);
                inst = inst->next;
            }
        }
    }
}

static void
ssa_convert(struct ir *file)
{
    struct uint_vector variables = vector_init();
    struct cfg_dfs_result dfs = cfg_dfs(&file->cfg); // NOTE: need postorder for DF
    
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
    struct uint_vector *phi_locations = malloc(variables.size * sizeof(struct uint_vector));
    
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
    
    for (u32 var_index = 0; var_index < variables.size; ++var_index) {
        if (store_blocks[var_index].size > 1) {
            phi_locations[var_index] = ssa_dominance_frontier(&file->cfg, &dfs, store_blocks + var_index);
        } else {
            phi_locations[var_index] = vector_init();
        }
    }
    
    
    struct int_stack versions = stack_init();
    u32 counter = 0;
    u32 mapping[100] = { 0 }; // TODO: count OpStores??
    
    // TODO: get actual OpVariables
    struct instruction_t original_variable;
    original_variable.wordcount = 4;
    original_variable.opcode = OpVariable;
    
    struct opvariable_t opvar = {
        .result_type = 0,
        .result_id = 1000,
        .storage_class = 0
    };
    
    original_variable.OpVariable = opvar;
    
    
    ssa_traverse(file, &versions, &counter, original_variable, mapping, 0);
}