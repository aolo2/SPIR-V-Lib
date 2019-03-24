// NOTE: returns -1 if the operand is certainly invariant, index
// of result id producer otherwise
static s32
invariant_or_locate(struct ir *file, struct lim_data *loop, u32 object_id)
{
    // NOTE: is the given operand constant
    if (search_item_u32(file->constants.data, file->constants.size, object_id) != -1) {
        return(-1);
    }
    
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

static bool
operand_invariant(struct ir *file, struct lim_data *loop, u32 pos)
{
    u32 opcode = loop->opcodes.data[pos];
    
    switch (opcode) {
        case OpStore: {
            u32 object_id  = loop->operands.data[2 * pos + 1];
            s32 object_pos = invariant_or_locate(file, loop, object_id);
            return(object_pos == -1 ? true : operand_invariant(file, loop, object_pos));
        } break;
        
        case OpLoad: {
            u32 pointer_id = loop->operands.data[2 * pos + 0];
            u32 n = loop->result_ids.size;
            
            for (s32 i = 0; i < n; ++i) {
                s32 other_pos = pos - 1 - i;
                
                if (other_pos < 0) {
                    other_pos += n;
                }
                
                if (loop->opcodes.data[other_pos] == OpStore) {
                    u32 other_store_pointer_id = loop->operands.data[2 * other_pos + 0];
                    if (other_store_pointer_id == pointer_id) {
                        return(other_pos > pos ? false : operand_invariant(file, loop, other_pos));
                    }
                }
            }
            
            return(true);
        } break;
        
        // NOTE: arithmetic operation
        default: {
            u32 operand_1 = loop->operands.data[2 * pos + 0];
            s32 operand_1_pos = invariant_or_locate(file, loop, operand_1);
            
            bool op1_inv = (operand_1_pos == -1 || operand_invariant(file, loop, operand_1_pos));
            
            if (!op1_inv) { return(false); }
            if (loop->operand_count.data[pos] == 1) { return(true); }
            
            u32 operand_2 = loop->operands.data[2 * pos + 1];
            s32 operand_2_pos = invariant_or_locate(file, loop, operand_2);
            
            return(operand_2_pos == -1 || operand_invariant(file, loop, operand_2_pos));
        }
    }
    
    SHOULDNOTHAPPEN;
}

static void
optimize_lim(struct uint_vector *bfs_order, struct ir *file)
{
    struct lim_data loop = {
        .invariants = vector_init(),
        .result_ids = vector_init(),
        .operand_count = vector_init(),
        .operands = vector_init(),
        .opcodes = vector_init()
    };
    
    // NOTE: prepare needed data
    for (u32 i = 0; i < bfs_order->size; ++i) {
        struct basic_block *block = file->blocks + bfs_order->data[i];
        for (u32 j = block->from; j < block->to; ++j) {
            struct instruction *inst = file->instructions + j;
            if (matters_in_cycle(inst->opcode)) {
                u32 op_count = extract_operand_count(inst);
                vector_push(&loop.opcodes, inst->opcode);
                vector_push(&loop.operand_count, op_count);
                vector_push(&loop.result_ids, extract_res_id(inst, file->words)); // NOTE: pushes 0 if no res id
                vector_push(&loop.operands, extract_nth_operand(inst, file->words, 0));
                vector_push(&loop.operands, op_count == 2 ? extract_nth_operand(inst, file->words, 1) : 0);
            }
        }
    }
    
    bool changes = true;
    
    while (changes) {
        changes = false;
        for (u32 i = 0; i < loop.result_ids.size; ++i) {
            if (loop.opcodes.data[i] == OpStore) {
                u32 operand = loop.operands.data[2 * i + 1];
                if (operand_invariant(file, &loop, i)) {
                    if (search_item_u32(loop.invariants.data, loop.invariants.size, operand) == -1) {
                        vector_push(&loop.invariants, operand);
                        changes = true;
                    }
                }
            }
        }
    }
    
    show_vector_u32(&loop.invariants, "invar");
    
    vector_free(&loop.invariants);
    vector_free(&loop.result_ids);
    vector_free(&loop.operand_count);
    vector_free(&loop.operands);
    vector_free(&loop.opcodes);
}