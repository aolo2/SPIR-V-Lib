static bool
is_invariant(struct uint_vector *invariant, struct uint_vector *expressions, 
             struct instruction_t *instructions, u32 operand)
{
    s32 invariant_index = search_item_u32(invariant->data, invariant->size, operand);
    s32 expression_index = search_item_u32(expressions->data, expressions->size, operand);
    
    if (invariant_index != -1 || expression_index == -1) {
        return(true);
    } else {
        struct instruction_t instruction = instructions[expression_index];
        switch (instruction.opcode) {
            case OpPhi: {
                return(false);
            } break;
            
            case OpLoad: {
                // NOTE: this has to be an OpLoad from an Input class variable
                return(true);
            } break;
            
            case OpCopyObject: {
                u32 operand = instruction.OpCopyObject.operand;
                return(is_invariant(invariant, expressions, instructions, operand));
            } break;
            
            default: {
                if (instruction.wordcount == 4) {
                    // unary arithmetics
                    u32 operand_1 = instruction.unary_arithmetics.operand;
                    return(is_invariant(invariant, expressions, instructions, operand_1));
                } else {
                    // binary arithmetics
                    u32 operand_1 = instruction.binary_arithmetics.operand_1;
                    u32 operand_2 = instruction.binary_arithmetics.operand_2;
                    
                    bool invariant_1 = is_invariant(invariant, expressions, instructions, operand_1);
                    bool invariant_2 = is_invariant(invariant, expressions, instructions, operand_2);
                    
                    return(invariant_1 && invariant_2);
                }
            }
        }
    }
}

static bool
mark_block(struct instruction_list **invariant, struct uint_vector *invariant_operands,
           struct uint_vector *expressions, struct instruction_t *instructions, struct basic_block *block)
{
    bool changes = false;
    struct instruction_list *instruction = block->instructions;
    
    while (instruction) {
        s32 object = -1;
        if (instruction->data.opcode == OpCopyObject) { 
            // NOTE: OpCopyObject is guranteed to back-reach all live operands
            // except for an OpStore to an Output class variable
            object = instruction->data.OpCopyObject.operand;
        } else if (instruction->data.opcode == OpStore) {
            // NOTE: OpStore to an Output class variable (that's the only one we generate)
            // is always supported by a single OpCopyObject producer
            u32 object = instruction->data.OpStore.object;
            s32 source_index = search_item_u32(expressions->data, expressions->size, object);
            ASSERT(source_index != -1);
            object = instructions[source_index].OpCopyObject.operand;
        }
        
        if (object != -1) {
            if (is_invariant(invariant_operands, expressions, instructions, object)) {
                if (vector_push_maybe(invariant_operands, object)) {
                    invariant[invariant_operands->head - 1] = instruction;
                }
                changes = true;
            }
        }
        
        instruction = instruction->next;
    }
    
    return(changes);
}

// NOTE: OpStore to this variable dominates all uses
// a.k.a. there are no phi functions with this pointer
// TODO: re-write for clean SSA
static bool
dom_uses(struct ir *file, struct uint_vector *bfs, u32 pointer)
{
    for (u32 block_index = 0; block_index < bfs->size; ++block_index) {
        struct basic_block *block = file->blocks + bfs->data[block_index];
        struct instruction_list *instruction = block->instructions;
        
        while (instruction) {
            if (instruction->data.opcode == OpPhi) {
                u32 var_count = (instruction->data.wordcount - 3) / 2;
                for (u32 var_index = 0; var_index < var_count; ++var_index) {
                    if (instruction->data.OpPhi.variables[var_index] == pointer) {
                        return(false);
                    }
                }
            }
            
            instruction = instruction->next;
        }
    }
    
    return(true);
}

// NOTE: OpStore dominates all exits
// TODO: re-write for clean SSA
static bool
dom_exits(struct ir *file, struct uint_vector *bfs, u32 var_block_index)
{
    struct basic_block header = file->blocks[bfs->data[0]];
    struct instruction_list *instruction = header.instructions;
    u32 merge_block = 0;
    
    while (instruction) {
        if (instruction->data.opcode == OpLoopMerge) {
            merge_block = instruction->data.OpLoopMerge.merge_block;
            break;
        }
        instruction = instruction->next;
    }
    
    ASSERT(merge_block != 0);
    u32 merge_block_index = search_item_u32(file->cfg.labels.data, 
                                            file->cfg.labels.size,
                                            merge_block);
    
    for (u32 i = 0; i < bfs->size; ++i) {
        u32 block_index = bfs->data[i];
        struct edge_list *edge = file->cfg.out[block_index];
        while (edge) {
            if (edge->data == merge_block_index) {
                if (!dominates(var_block_index, block_index, file->cfg.dominators)) {
                    return(false);
                }
            }
            edge = edge->next;
        }
    }
    
    return(true);
}

static void
process_loop(struct ir *file, u32 header_block, u32 merge_block)
{
    struct uint_vector bfs = cfg_bfs_order_r(&file->cfg, header_block, merge_block);
    struct uint_vector expressions = vector_init();
    struct instruction_t instructions[128]; // TODO: count and malloc
    
    // NOTE: collect 'expressions' from cycle
    for (u32 block_index = 0; block_index < bfs.size; ++block_index) {
        struct basic_block *block = file->blocks + bfs.data[block_index];
        struct instruction_list *instruction = block->instructions;
        while (instruction) {
            enum opcode_t opcode = instruction->data.opcode;
            if (produces_result_id(opcode)) {
                instructions[expressions.head] = instruction->data;
                vector_push(&expressions, get_result_id(&instruction->data));
            }
            instruction = instruction->next;
        }
    }
    
    bool changes = true;
    struct instruction_list *invariant[100];  // TODO: count and malloc
    struct uint_vector invariant_operands = vector_init();
    struct uint_vector blocks = vector_init();
    
    while (changes) {
        changes = false;
        for (u32 i = 0; i < bfs.size; ++i) {
            u32 block_index = bfs.data[i];
            struct basic_block *block = file->blocks + block_index;
            u32 last_invariant_size = invariant_operands.size;
            
            if (mark_block(invariant, &invariant_operands, 
                           &expressions, instructions, block) && invariant_operands.size > last_invariant_size) {
                // NOTE: we will later need to know the basic block for each invariant operand
                while (last_invariant_size != invariant_operands.size) {
                    vector_push(&blocks, block_index);
                    ++last_invariant_size;
                }
                
                changes = true;
            }
        }
    }
    
    if (invariant_operands.size > 0) {
        u32 header_index = bfs.data[0];
        u32 preheader_index = ir_add_bb(file);
        struct basic_block *preheader = file->blocks + preheader_index;
        
        for (u32 i = 0; i < invariant_operands.size; ++i) {
            bool cond1 = dom_uses(file, &bfs, blocks.data[i]);
            //bool cond2 = dom_exits(&file, &bfs, blocks.data[i]);
            
            if (cond1) {
                struct basic_block *block = file->blocks + blocks.data[i];
                ir_append_instruction(preheader, invariant[i]->data);
                ir_delete_instruction(block, invariant[i]);
            }
        }
        
        // NOTE: redirect all header incoming edges (but not from the loop itself!)
        struct uint_vector header_incoming = vector_init();
        struct edge_list *edge = file->cfg.in[header_index];
        while (edge) {
            u32 from = edge->data;
            if (search_item_u32(bfs.data, bfs.size, from) == -1) {
                vector_push(&header_incoming, edge->data);
            }
            edge = edge->next;
        }
        
        for (u32 i = 0; i < header_incoming.size; ++i) {
            cfg_redirect_edge(&file->cfg, header_incoming.data[i], header_index, preheader_index);
        }
        
        // NOTE: make an edge from preheader to header
        cfg_add_edge(&file->cfg, preheader_index, header_index);
    }
    
    vector_free(&invariant_operands);
    vector_free(&bfs);
    vector_free(&expressions);
    vector_free(&blocks);
}

static void
loop_invariant_code_motion(struct ir *file) 
{
    // NOTE: locate all loops (header and merge block 
    // identify a structured loop)
    for (u32 block_index = 0; block_index < file->cfg.labels.size; ++block_index) {
        struct instruction_list *inst = file->blocks[block_index].instructions;
        while (inst) {
            if (inst->data.opcode == OpLoopMerge) {
                u32 header_block = block_index;
                u32 merge_block = inst->data.OpLoopMerge.merge_block;
                process_loop(file, header_block, merge_block);
                break;
            }
            inst = inst->next;
        }
    }
}