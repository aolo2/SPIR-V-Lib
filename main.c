#include "headers.h"

static u32 *
get_binary(const char *filename, u32 *size)
{
    FILE *file = fopen(filename, "rb");
    
    if (!file) {
        fprintf(stderr, "[ERROR] File could not be opened\n");
        return(NULL);
    }
    
    fseek(file, 0L, SEEK_END);
    *size = ftell(file);
    rewind(file);
    
    // NOTE: size % sizeof(u32) is always zero
    u32 *buffer = (u32 *) malloc(*size);
    fread((u8 *) buffer, *size, 1, file);
    
    *size /= sizeof(u32);
    
    fclose(file);
    
    return(buffer);
}

static bool
is_invariant(struct uint_vector *invariant, struct uint_vector *expressions, u32 operand)
{
    if (search_item_u32(invariant->data, invariant->size, operand) != -1 || search_item_u32(expressions->data, expressions->size, operand) == -1) {
        return(true);
    }
    
    return(false);
}

// TODO: get actual invariant instructions, not just res id
static bool
mark_block(struct uint_vector *invariant, struct uint_vector *expressions, struct basic_block *block)
{
    bool changes = false;
    struct instruction_list *instruction = block->instructions;
    
    while (instruction) {
        switch (instruction->data.opcode) {
            case OpLoad: {
                u32 operand = instruction->data.OpLoad.pointer;bool is_inv = is_invariant(invariant, expressions, operand);
                if (is_inv) {
                    vector_push_maybe(invariant, instruction->data.OpLoad.result_id);
                    changes = true;
                }
            } break;
            
            case OpPhi: {
                u32 operand_count = (instruction->data.wordcount - 3) / 2;
                bool all_inv = true;
                
                for (u32 operand_index = 0; operand_index < operand_count; ++operand_index) {
                    u32 operand = instruction->data.OpPhi.variables[operand_index];
                    bool is_inv = is_invariant(invariant, expressions, operand);
                    if (!is_inv) {
                        all_inv = false;
                        break;
                    }
                }
                
                if (all_inv) {
                    vector_push_maybe(invariant, instruction->data.OpPhi.result_id);
                }
            } break;
            
            case OpStore: {
                u32 operand = instruction->data.OpStore.object;
                bool is_inv = is_invariant(invariant, expressions, operand);
                if (is_inv) {
                    vector_push_maybe(invariant, instruction->data.OpStore.pointer);
                    changes = true;
                }
            } break;
            
            case OpSNegate:
            case OpFNegate:
            case OpIAdd:
            case OpFAdd:
            case OpISub:
            case OpFSub:
            case OpIMul:
            case OpFMul:
            case OpUDiv:
            case OpSDiv:
            case OpFDiv:
            case OpUMod:
            case OpSRem:
            case OpSMod:
            case OpFRem:
            case OpFMod: {
                if (instruction->data.wordcount == 4) {
                    // unary arithmetics
                    u32 operand = instruction->data.unary_arithmetics.operand;
                    bool is_inv = is_invariant(invariant, expressions, operand);
                    if (is_inv) {
                        vector_push_maybe(invariant, instruction->data.unary_arithmetics.result_id);
                        changes = true;
                    }
                } else {
                    // binary arithmetics
                    u32 operand_1 = instruction->data.binary_arithmetics.operand_1;
                    u32 operand_2 = instruction->data.binary_arithmetics.operand_2;
                    bool is_inv_1 = is_invariant(invariant, expressions, operand_1);
                    bool is_inv_2 = is_invariant(invariant, expressions, operand_2);
                    if (is_inv_1 && is_inv_2) {
                        vector_push_maybe(invariant, instruction->data.binary_arithmetics.result_id);
                        changes = true;
                    }
                }
            } break;
            
            default: {
                // SKIP
            }
        }
        instruction = instruction->next;
    }
    
    return(changes);
}

s32
main(void)
{
    u32 num_words;
    u32 *words = get_binary("data/cycle.frag.spv", &num_words);
    struct ir file = ir_eat(words, num_words);
    
    // NOTE: for any OpVariable there is only ONE OpStore (and maybe one OpLoad)
    ssa_convert(&file);
    
#if 0
    // NOTE: Example: Loop Invariant Code Motion
    struct uint_vector bfs = cfg_bfs_order_r(&file.cfg, 1, 7);
    struct uint_vector invariant = vector_init();
    struct uint_vector expressions = vector_init();
    
    // collect 'expressions' from cycle
    for (u32 block_index = 0; block_index < bfs.size; ++block_index) {
        struct basic_block *block = file.blocks + bfs.data[block_index];
        struct instruction_list *instruction = block->instructions;
        while (instruction) {
            enum opcode_t opcode = instruction->data.opcode;
            if (produces_result_id(opcode)) {
                vector_push(&expressions, get_result_id(&instruction->data));
            } else if (opcode == OpStore) {
                vector_push(&expressions, instruction->data.OpStore.pointer);
            }
            instruction = instruction->next;
        }
    }
    
    bool changes = true;
    
    while (changes) {
        changes = false;
        for (u32 block_index = 0; block_index < bfs.size; ++block_index) {
            struct basic_block *block = file.blocks + bfs.data[block_index];
            u32 last_invariant_size = invariant.size;
            if (mark_block(&invariant, &expressions, block) && invariant.size > last_invariant_size) {
                changes = true;
            }
        }
    }
    
    vector_free(&bfs);
    vector_free(&invariant);
    vector_free(&expressions);
#endif
    
    ir_dump(&file, "data/cycle2.frag.spv");
    ir_destroy(&file);
    
    return(0);
}