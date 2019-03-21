// A: what is a constant operand in opstore??
// uniform
// constant - opconstanttrue / opconstantfalse / opconstant / opconstantcomposite / opconstantnull (?)

// need to create two lists for linear search
// constant res_id's and uniform res_id's
// check against that list

// Q: what is outdef?
// need a list of declarations (ids) 
// AND OTHER RES_ID PRODUCING INSTRUCTIONS!
//
// mb just a list of res_id's produced in the cycle body?
// 
// inside the cycle to 
// check against it. not in list -- outdef

// Q: what is indef?
// check against a list of declarations again. if found one -- check it
// again. const or outdef -- good, else bad

// what is a declaration???

// produce a 'loop' by starting from OpLoopMerge, getting the merge_block,
// and running a bfs (why? says so. =) ), not going to the merge_block edges
// this produces an order of bb indices. 
// These indices we call a loop.

// also, when performing the bfs, collect all instructions with a res id (from a 
// subset ofcourse, that would be 

// OpVariable, OpLoad
// arithmetics OpSNegate, OpFNegate, OpIAdd, OpFAdd, OpISub, OpFSub, OpIMul, OpFMul, OpUDiv, OpSDiv, OpFDiv, OpUMod, OpSRem, OpSMod, OpFRem, OpFMod)
// TODO: composite extract/construct
// these are our 'assignments' in the cycle
// 

#if 0

static bool
is_in_def(o)
{
}

static bool
mark_block(v)
{
    for stmt {
        for operand {
            if (is_const(o) || is_out_def(o) || is_in_def(o)) {
                
            }
        }
    }
}
#endif

static inline bool
is_out_def(struct bfs_result *loop, u32 operand)
{
    return(search_item_u32(loop->ids.data, loop->ids.size, operand) == -1);
}

static inline bool
is_constant(struct uint_vector *constants, u32 operand)
{
    return(search_item_u32(constants->data, constants->size, operand) != -1);
}

static inline bool
is_in_def(struct uint_vector *invariants, u32 operand)
{
    return(search_item_u32(invariants->data, invariants->size, operand) != -1);
}

static void
optimize_lim(struct bfs_result *loop, struct uint_vector *constants,
             struct basic_block *blocks, struct instruction *instructions, u32 *words)
{
    printf("\n=== loop ===\n");
    
    struct uint_vector invariants = vector_init();
    struct uint_vector stores = vector_init();
    
    for (u32 i = 0; i < loop->sorted.size; ++i) {
        // NOTE: find all OpStores and check them operands
        struct basic_block *block = blocks + loop->sorted.data[i];
        for (u32 j = block->from; j < block->to; ++j) {
            struct instruction *inst = instructions + j;
            if (inst->opcode == OpStore) {
                
                u32 variable = words[inst->words_from + OPSTORE_POINTER_INDEX];
                // TODO!!!
                
                vector_push(&stores, j);
            }
        }
    }
    
    bool changed = true;
    
    while (changed) {
        changed = false;
        for (u32 i = 0; i < stores.size; ++i) {
            u32 inst = stores.data[i];
            u32 operand = words[instructions[inst].words_from + OPSTORE_OBJECT_INDEX];
            
            if (is_constant(constants, operand) || is_out_def(loop, operand) || is_in_def(&invariants, operand)) {
                if (search_item_u32(invariants.data, invariants.size, inst) == -1) {
                    vector_push(&invariants, inst);
                    changed = true;
                }
            }
        }
    }
    
    show_vector_u32(&invariants, "invar");
    
    vector_free(&invariants);
}



// a = b
// b = a


// opstore a, invar
// 