static bool
dominates(u32 parent, s32 child, s32 *dominators)
{
    if (parent == child) {
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

static bool
dominates_all_uses(struct ir *file, struct lim_data *loop, u32 pos)
{
    u32 inst_pos = loop->instructions.data[pos];
    u32 block_index = loop->blocks.data[pos];
    
    struct uint_vector dfs_order = cfgc_dfs_restricted(&file->cfg, block_index, loop->merge_block_index);
    struct instruction *store = file->instructions + inst_pos;
    
    if (store->opcode == OpStore) {
        u32 pointer_id = file->words[store->words_from + OPSTORE_POINTER_INDEX];
        for (u32 i = 1; i < dfs_order.size; ++i) {
            u32 other_block_index = dfs_order.data[i];
            struct basic_block *block = file->blocks + other_block_index;
            for (u32 j = block->from; j < block->to; ++j) {
                struct instruction *inst = file->instructions + j;
                if (inst->opcode == OpLoad) {
                    if (file->words[inst->words_from + OPLOAD_POINTER_INDEX] == pointer_id) {
                        // NOTE: this is an OpLoad from our pointer, is it dominated by the OpStore?
                        if (!dominates(block_index, other_block_index, file->dominators)) {
                            return(false);
                        }
                    }
                }
                
                // NOTE: the next OpStore to the same pointer, do not search further
                if (inst->opcode == OpStore) {
                    if (file->words[inst->words_from + OPSTORE_POINTER_INDEX] == pointer_id) {
                        return(true);
                    }
                }
            }
        }
        
        return(true);
    } else {
        // NOTE: arithmetics can not NOT dominate the use
        // OpLoad is checked in the try_add_invariant routine
        return(true);
    }
}

// NOTE: returns -1 if the operand is certainly invariant, index
// of result id producer otherwise
static s32
invariant_or_locate(struct lim_data *loop, u32 object_id)
{
    // NOTE: is already found to be invariant
    if (search_item_u32(loop->invariants.data, loop->invariants.size, object_id) != -1) {
        return(-1);
    }
    
    for (u32 i = 0; i < loop->result_ids.size; ++i) {
        if (loop->result_ids.data[i] == object_id) {
            return(i);
        }
    }
    
    return(-1);
}

static inline void
try_push(struct uint_vector *v, u32 item)
{
    if (search_item_u32(v->data, v->size, item) == -1) {
        vector_push(v, item);
    }
}

static bool
try_add_invariant(struct ir *file, struct lim_data *loop, u32 pos)
{
    u32 opcode = loop->opcodes.data[pos];
    
    switch (opcode) {
        case OpStore: {
            u32 object_id  = loop->operands.data[2 * pos + 1];
            s32 object_pos = invariant_or_locate(loop, object_id);
            
            bool res = object_pos == -1 ? true : try_add_invariant(file, loop, object_pos);
            if (res) { try_push(&loop->invariants, pos); }
            
            return(res);
        } break;
        
        case OpLoad: {
            // NOTE: find the last Opstore to this pointer
            u32 load_block_index = loop->blocks.data[pos];
            
            u32 pointer_id = loop->operands.data[2 * pos + 0];
            u32 n = loop->result_ids.size;
            
            for (s32 i = 0; i < (s32) n; ++i) {
                s32 other_pos = pos - 1 - i;
                
                if (other_pos < 0) {
                    other_pos += n;
                }
                
                if (loop->opcodes.data[other_pos] == OpStore) {
                    u32 other_store_pointer_id = loop->operands.data[2 * other_pos + 0];
                    if (other_store_pointer_id == pointer_id) {
                        u32 store_block_index = loop->blocks.data[other_pos];
                        bool store_dom_load = dominates(store_block_index, load_block_index, file->dominators);
                        bool last_iteration = other_pos > (s32) pos;
                        
                        if (!store_dom_load || last_iteration) {
                            return(false);
                        }
                        
                        bool res = try_add_invariant(file, loop, other_pos);
                        if (res) { try_push(&loop->invariants, pos); }
                        
                        return(res);
                    }
                }
            }
            
            try_push(&loop->invariants, pos);
            
            return(true);
        } break;
        
        // NOTE: arithmetic operation
        default: {
            u32 operand_1 = loop->operands.data[2 * pos + 0];
            s32 operand_1_pos = invariant_or_locate(loop, operand_1);
            
            bool op1_inv = (operand_1_pos == -1 || try_add_invariant(file, loop, operand_1_pos));
            
            if (!op1_inv) { return(false); }
            if (loop->operand_count.data[pos] == 1) { try_push(&loop->invariants, pos); }
            
            u32 operand_2 = loop->operands.data[2 * pos + 1];
            s32 operand_2_pos = invariant_or_locate(loop, operand_2);
            
            bool res = operand_2_pos == -1 || try_add_invariant(file, loop, operand_2_pos);
            if (res) { try_push(&loop->invariants, pos); }
            
            return(res);
        }
    }
}

// NOTE: we can cheat quite severely here by not modifying the full IR:
//    - adding a preheader does not alter the control flow
// 
// Because of this it's enough to just add the words of the new basic block
// near the end of the file. BUT we need to add an OpBranch to the predecessor
// of the loop!
static void
code_motion(struct ir *file, struct lim_data *loop, struct uint_vector *motion)
{
    struct basic_block *last_block = file->blocks + file->num_bb - 1;
    u32 tail_from = file->instructions[last_block->to].words_from;
    u32 tail_to = file->num_words;
    u32 tail_size = tail_to - tail_from;
    
    u32 block_words = OPLABEL_WORDCOUNT + OPBRANCH_WORDCOUNT;
    
    for (u32 i = 0; i < motion->size; ++i) {
        u32 index = motion->data[i];
        u32 instruction_index = loop->instructions.data[index];
        struct instruction *inst = file->instructions + instruction_index;
        block_words += inst->wordcount;
    }
    
    file->num_words += block_words;
    file->words = realloc(file->words, file->num_words * WORD_SIZE);
    
    // NOTE: copy the trailing instructions first so that we do not override them
    // IMPORTNANT: pointer arithmetic
    memmove(file->words + tail_from + block_words, 
            file->words + tail_from, tail_size * WORD_SIZE);
    
    // NOTE: add the new basic block
    u32 *word = file->words + tail_from;
    *(word++) = OPLABEL_WORD0;
    *(word++) = file->header.bound;
    
    ++file->header.bound;
    
    for (u32 i = 0; i < motion->size; ++i) {
        u32 instruction_index = loop->instructions.data[motion->data[i]];
        struct instruction *inst = file->instructions + instruction_index;
        for (u32 j = 0; j < inst->wordcount; ++j) {
            *(word++) = file->words[inst->words_from + j];
        }
    }
    
    *(word + 0) = OPBRANCH_WORD0;
    *(word + 1) = loop->entry_block_id;
    
    // NOTE: remove old instructions (splice)
    for (u32 i = 0; i < motion->size; ++i) {
        u32 instruction_index = loop->instructions.data[motion->data[i]];
        struct instruction *inst = file->instructions + instruction_index;
        
        u32 *words_from = file->words + inst->words_from;
        u32 tail_words = file->num_words - (inst->words_from + inst->wordcount);
        
        memmove(words_from, words_from + inst->wordcount, tail_words * WORD_SIZE);
        
        --file->num_instructions;
        
        for (u32 j = instruction_index; j < file->num_instructions; ++j) {
            file->instructions[j].words_from -= inst->wordcount;
        }
        
        file->num_words -= inst->wordcount;
    }
    
#if 1
    // NOTE: replace all OpBranch-es to OpLoopMerge block
    // with branches to pre-header (but not the 'continue' edge)
    for (u32 i = 0; i < file->num_bb; ++i) {
        struct basic_block *block = file->blocks + i;
        for (u32 j = block->from; j < block->to; ++j) {
            struct instruction *inst = file->instructions + j;
            if (inst->opcode == OpBranch && block->id != loop->continue_block_id) {
                u32 target = file->words[inst->words_from + OPBRANCH_TARGETLABEL_INDEX];
                if (target == loop->entry_block_id) {
                    file->words[inst->words_from + OPBRANCH_TARGETLABEL_INDEX] = file->header.bound - 1;
                }
            }
        }
    }
#endif
}

static void
optimize_lim(struct uint_vector *bfs_order, struct ir *file)
{
    struct lim_data loop = {
        .invariants = vector_init(),
        .result_ids = vector_init(),
        .operand_count = vector_init(),
        .operands = vector_init(),
        .opcodes = vector_init(),
        .blocks = vector_init(),
        .instructions = vector_init(),
        .multiple_exits = false
    };
    
    // NOTE: prepare needed data
    u32 continue_count = 0;
    for (u32 i = 0; i < bfs_order->size; ++i) {
        struct basic_block *block = file->blocks + bfs_order->data[i];
        for (u32 j = block->from; j < block->to; ++j) {
            struct instruction *inst = file->instructions + j;
            
            if (inst->opcode == OpLoopMerge) {
                loop.entry_block_id    = block->id;
                loop.merge_block_id    = file->words[inst->words_from + OPLOOPMERGE_MERGE_INDEX];
                loop.continue_block_id = file->words[inst->words_from + OPLOOPMERGE_CONTINUE_INDEX];
                
                loop.merge_block_index = search_item_u32(file->bb_labels, file->num_bb, loop.merge_block_id);
                loop.entry_block_index = search_item_u32(file->bb_labels, file->num_bb, loop.entry_block_id);
            }
            
            if (!loop.multiple_exits && inst->opcode == OpBranch) {
                u32 target = file->words[inst->words_from + OPBRANCH_TARGETLABEL_INDEX];
                if (target == loop.merge_block_id) {
                    loop.multiple_exits = true;
                } else if (target == loop.continue_block_id) {
                    ++continue_count;
                    if (continue_count > 1) {
                        loop.multiple_exits = true;
                    }
                }
            }
            
            if (matters_in_cycle(inst->opcode)) {
                u32 op_count = extract_operand_count(inst);
                vector_push(&loop.opcodes, inst->opcode);
                vector_push(&loop.operand_count, op_count);
                vector_push(&loop.operands, extract_nth_operand(inst, file->words, 0));
                vector_push(&loop.result_ids, extract_res_id(inst, file->words));
                vector_push(&loop.operands,op_count == 2 ? extract_nth_operand(inst, file->words, 1) : 0);
                vector_push(&loop.blocks, bfs_order->data[i]);
                vector_push(&loop.instructions, j);
            }
        }
    }
    
    bool changes = true;
    
    while (changes) {
        changes = false;
        for (u32 i = 0; i < loop.result_ids.size; ++i) {
            if (loop.opcodes.data[i] == OpStore) {
                u32 old_invariants_size = loop.invariants.size;
                if (try_add_invariant(file, &loop, i)) {
                    if (old_invariants_size != loop.invariants.size)
                        changes = true;
                }
            }
        }
    }
    
    struct uint_vector motion = vector_init();
    
    for (u32 i = 0; i < loop.invariants.size; ++i) {
        u32 inv = loop.invariants.data[i];
        
        bool dom_ae = !loop.multiple_exits;
        bool dom_au = dominates_all_uses(file, &loop, inv);  
        
        if (dom_ae && dom_au) {
            vector_push(&motion, inv);
        }
    }
    
    code_motion(file, &loop, &motion);
    
    vector_free(&motion);
    vector_free(&loop.invariants);
    vector_free(&loop.result_ids);
    vector_free(&loop.operand_count);
    vector_free(&loop.operands);
    vector_free(&loop.opcodes);
}