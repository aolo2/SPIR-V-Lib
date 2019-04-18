static const u32 WORDCOUNT_MASK     = 0xFFFF0000;
static const u32 OPCODE_MASK        = 0x0000FFFF;

enum opcode_t {
    OpVariable = 59,
    OpLoad = 61,
    OpStore = 62,
    OpSNegate = 126,
    OpFNegate = 127,
    OpIAdd = 128,
    OpFAdd = 129,
    OpISub = 130,
    OpFSub = 131,
    OpIMul = 132,
    OpFMul = 133,
    OpUDiv = 134,
    OpSDiv = 135,
    OpFDiv = 136,
    OpUMod = 137,
    OpSRem = 138,
    OpSMod = 139,
    OpFRem = 140,
    OpFMod = 141,
    OpPhi = 245,
    OpLoopMerge = 246,
    OpSelectionMerge = 247,
    OpLabel = 248,
    OpBranch = 249,
    OpBranchConditional = 250,
    OpReturn = 253,
};

struct oplabel_t {
    u32 result_id;
};

struct opvariable_t {
    u32 result_type;
    u32 result_id;
    u32 storage_class;
    u32 initializer; // optional
};

struct opbranch_t {
    u32 target_label;
};

struct opbranchconditional_t {
    u32 condition;
    u32 true_label;
    u32 false_label;
    // does not support label weights
};

struct opselectionmerge_t {
    u32 merge_block;
    u32 selection_control;
};

struct opphi_t {
    u32 result_type;
    u32 result_id;
    u32 *variables;
    u32 *parents;
};

struct oploopmerge_t {
    u32 merge_block;
    u32 continue_block;
    u32 loop_control;
};

struct opstore_t {
    u32 pointer;
    u32 object;
    u32 memory_access; // optional
};

struct opload_t {
    u32 result_type;
    u32 result_id;
    u32 pointer;
    u32 memory_access; // optional
};

struct unary_arithmetics_layout {
    u32 result_type;
    u32 result_id;
    u32 operand;
};

struct binary_arithmetics_layout {
    u32 result_type;
    u32 result_id;
    u32 operand_1;
    u32 operand_2;
};

struct instruction_t {
    enum opcode_t opcode;
    u32 wordcount;
    u32 *unparsed_words;
    
    union {
        struct oplabel_t OpLabel;
        struct opvariable_t OpVariable;
        struct opbranch_t OpBranch;
        struct opbranchconditional_t OpBranchConditional;
        struct opphi_t OpPhi;
        struct opselectionmerge_t OpSelectionMerge;
        struct oploopmerge_t OpLoopMerge;
        struct opstore_t OpStore;
        struct opload_t OpLoad;
        struct unary_arithmetics_layout unary_arithmetics;
        struct binary_arithmetics_layout binary_arithmetics;
    };
};

static bool
supported_in_cfg(enum opcode_t opcode)
{
    switch (opcode) {
        case OpVariable:
        case OpLoad:
        case OpStore:
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
        case OpFMod:
        case OpPhi:
        case OpLoopMerge:
        case OpSelectionMerge:
        case OpLabel:
        case OpBranch:
        case OpBranchConditional:
        case OpReturn: {
            return(true);
        }
        
        default: {
            return(false);
        }
    }
}

static bool
terminal(enum opcode_t opcode)
{
    return(opcode == OpBranch || opcode == OpBranchConditional || opcode == OpReturn);
}

static struct instruction_t
get_instruction(u32 *word)
{
    struct instruction_t instruction;
    instruction.opcode = *word & OPCODE_MASK,
    instruction.wordcount = (*word & WORDCOUNT_MASK) >> 16,
    instruction.unparsed_words = memdup(word, instruction.wordcount * 4);
    
    // NOTE: move the pointer to the first word of the first instruction
    ++word;
    
    switch (instruction.opcode) {
        case OpReturn: {
        }break;
        
        case OpLabel: {
            instruction.OpLabel.result_id = *word;
        } break;
        
        case OpVariable: {
            instruction.OpVariable.result_type = *(word++);
            instruction.OpVariable.result_id = *(word++);
            instruction.OpVariable.storage_class = *(word++);
            if (instruction.wordcount == 5) {
                instruction.OpVariable.result_type = *(word++);
            }
        } break;
        
        case OpBranch: {
            instruction.OpBranch.target_label = *(word++);
        } break;
        
        case OpBranchConditional: {
            instruction.OpBranchConditional.condition = *(word++);
            instruction.OpBranchConditional.true_label = *(word++);
            instruction.OpBranchConditional.false_label = *(word++);
        } break;
        
        case OpPhi: {
            instruction.OpPhi.result_type = *(word++);
            instruction.OpPhi.result_id = *(word++);
            instruction.OpPhi.variables = malloc((instruction.wordcount - 3) / 2 * sizeof(u32));
            instruction.OpPhi.parents = malloc((instruction.wordcount - 3) / 2 * sizeof(u32));
            for (u32 i = 0; i < instruction.wordcount - 3; ++i) {
                instruction.OpPhi.variables[i] = *(word++);
                instruction.OpPhi.parents[i] = *(word++);
            }
        } break;
        
        case OpSelectionMerge: {
            instruction.OpSelectionMerge.merge_block = *(word++);
            instruction.OpSelectionMerge.selection_control = *(word++);
        } break;
        
        case OpLoopMerge: {
            instruction.OpLoopMerge.merge_block = *(word++);
            instruction.OpLoopMerge.continue_block = *(word++);
            instruction.OpLoopMerge.loop_control = *(word++);
        } break;
        
        case OpStore: {
            instruction.OpStore.pointer = *(word++);
            instruction.OpStore.object = *(word++);
            if (instruction.wordcount == 4) {
                instruction.OpStore.memory_access = *(word++);
            }
        } break;
        
        case OpLoad: {
            instruction.OpLoad.result_type = *(word++);
            instruction.OpLoad.result_id = *(word++);
            instruction.OpLoad.pointer = *(word++);
            if (instruction.wordcount == 5) {
                instruction.OpLoad.memory_access = *(word++);
            }
        } break;
        
        case OpSNegate: 
        case OpFNegate: {
            instruction.unary_arithmetics.result_type = *(word++);
            instruction.unary_arithmetics.result_id = *(word++);
            instruction.unary_arithmetics.operand = *(word++);
        } break;
        
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
            instruction.binary_arithmetics.result_type = *(word++);
            instruction.binary_arithmetics.result_id = *(word++);
            instruction.binary_arithmetics.operand_1 = *(word++);
            instruction.binary_arithmetics.operand_2 = *(word++);
        } break;
    }
    
    return(instruction);
}

static u32 *
dump_instruction(struct instruction_t *inst, u32 *buffer)
{
    buffer[0] = inst->opcode | (inst->wordcount << 16);
    
    switch (inst->opcode) {
        case OpReturn: {
        } break;
        
        case OpLabel: {
            buffer[1] = inst->OpLabel.result_id;
        } break;
        
        case OpVariable: {
            buffer[1] = inst->OpVariable.result_type;
            buffer[2] = inst->OpVariable.result_id;
            buffer[3] = inst->OpVariable.storage_class;
            if (inst->wordcount == 5) {
                buffer[4] = inst->OpVariable.result_type;
            }
        } break;
        
        case OpBranch: {
            buffer[1] = inst->OpBranch.target_label;
        } break;
        
        case OpBranchConditional: {
            buffer[1] = inst->OpBranchConditional.condition;
            buffer[2] = inst->OpBranchConditional.true_label;
            buffer[3] = inst->OpBranchConditional.false_label;
        } break;
        
        case OpPhi: {
            buffer[1] = inst->OpPhi.result_type;
            buffer[2] = inst->OpPhi.result_id;
            for (u32 i = 3 ; i < inst->wordcount; i += 2) {
                buffer[i + 0] = inst->OpPhi.variables[(i - 3) / 2];
                buffer[i + 1] = inst->OpPhi.parents[(i - 3) / 2];
            }
        } break;
        
        case OpSelectionMerge: {
            buffer[1] = inst->OpSelectionMerge.merge_block;
            buffer[2] = inst->OpSelectionMerge.selection_control;
        } break;
        
        case OpLoopMerge: {
            buffer[1] = inst->OpLoopMerge.merge_block;
            buffer[2] = inst->OpLoopMerge.continue_block;
            buffer[3] = inst->OpLoopMerge.loop_control;
        } break;
        
        case OpStore: {
            buffer[1] = inst->OpStore.pointer;
            buffer[2] = inst->OpStore.object;
            if (inst->wordcount == 4) {
                buffer[3] = inst->OpStore.memory_access;
            }
        } break;
        
        case OpLoad: {
            buffer[1] = inst->OpLoad.result_type;
            buffer[2] = inst->OpLoad.result_id;
            buffer[3] = inst->OpLoad.pointer;
            if (inst->wordcount == 5) {
                buffer[4] = inst->OpLoad.memory_access;
            }
        } break;
        
        case OpSNegate: 
        case OpFNegate: {
            buffer[1] = inst->unary_arithmetics.result_type;
            buffer[2] = inst->unary_arithmetics.result_id;
            buffer[3] = inst->unary_arithmetics.operand;
        } break;
        
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
            buffer[1] = inst->binary_arithmetics.result_type;
            buffer[2] = inst->binary_arithmetics.result_id;
            buffer[3] = inst->binary_arithmetics.operand_1;
            buffer[4] = inst->binary_arithmetics.operand_2;
        } break;
        
        default: {
            memcpy(buffer, inst->unparsed_words, inst->wordcount * 4);
        }
    }
    
    return(buffer);
}

static bool
produces_result_id(enum opcode_t opcode)
{
    switch (opcode) {
        case OpVariable:
        case OpLoad:
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
        case OpFMod:
        case OpPhi:
        case OpLabel: {
            return(true);
        }
        
        case OpLoopMerge:
        case OpSelectionMerge:
        case OpBranch:
        case OpBranchConditional:
        case OpReturn:
        case OpStore: {
            return(false);
        }
    }
    
    SHOULDNOTHAPPEN;
}

static u32
get_result_id(struct instruction_t *instruction)
{
    switch (instruction->opcode) {
        case OpVariable: {
            return(instruction->OpVariable.result_id);
        }
        
        case OpLoad: {
            return(instruction->OpLoad.result_id);
        }
        
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
            return(instruction->binary_arithmetics.result_id);
        }
        
        case OpPhi: {
            return(instruction->OpPhi.result_id);
        }
        
        case OpLabel: {
            return(instruction->OpLabel.result_id);
        }
    }
    
    SHOULDNOTHAPPEN;
}