#include "common.h"

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
        struct opselectionmerge_t OpSelectionMerge;
        struct oploopmerge_t OpLoopMerge;
        struct opstore_t OpStore;
        struct opload_t OpLoad;
        struct unary_arithmetics_layout unary_arithmetics;
        struct binary_arithmetics_layout binary_arithmetics;
    };
};

struct ir_header {
    u32 magic_number;
    u32 version;
    u32 generator_mn;
    u32 bound;
    u32 zero;
};

struct instruction_list {
    struct instruction_t data;
    struct instruction_list *next;
    struct instruction_list *prev;
};

struct basic_block {
    u32 count;
    struct instruction_list *instructions;
};

struct ir {
    struct ir_header header;
    struct ir_cfg cfg;
    struct basic_block *blocks;
    struct instruction_list *pre_cfg;
    struct instruction_list *post_cfg;
};

static const u32 WORDCOUNT_MASK     = 0xFFFF0000;
static const u32 OPCODE_MASK        = 0x0000FFFF;

static bool
terminal(enum opcode_t opcode)
{
    return(opcode == OpBranch || opcode == OpBranchConditional || opcode == OpReturn);
}

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

static inline s32
search_item_u32(u32 *array, u32 count, u32 item)
{
    for (u32 i = 0; i < count; ++i) {
        if (array[i] == item) {
            return(i);
        }
    }
    return(-1);
}

static struct ir
eat_ir(u32 *data, u32 size)
{
    u32 labels[100] = { 0 };
    u32 bb_count = 0;
    
    struct ir file;
    file.header = *((struct ir_header *) data);
    
    struct instruction_list *all_instructions;
    struct instruction_list *inst;
    struct instruction_list *last = NULL;
    
    u32 offset = sizeof(struct ir_header) / 4;
    u32 *word = data + offset;
    
    // NOTE: get all instructions in a list, count basic blocks
    do {
        inst = malloc(sizeof(struct instruction_list));
        inst->data = get_instruction(word);
        inst->prev = last;
        
        if (inst->data.opcode == OpLabel) {
            labels[bb_count++] = inst->data.OpLabel.result_id;
        }
        
        offset += inst->data.wordcount;
        
        if (last) {
            last->next = inst;
        } else {
            all_instructions = inst;
        }
        
        last = inst;
        word = data + offset;
    } while (offset != size);
    
    last->next = NULL;
    // =======================
    
    
    file.blocks = malloc(sizeof(struct basic_block) * bb_count);
    
    
    // NOTE: all pre-cfg instructions
    file.pre_cfg = all_instructions;
    inst = all_instructions;
    do {
        inst = inst->next;
    } while (inst->data.opcode != OpLabel);
    
    inst->prev->next = NULL;
    inst->prev = NULL;
    // =======================
    
    
    // NOTE: reconstruct the cfg
    file.cfg = cfg_init(labels, bb_count);
    
    struct basic_block block;
    u32 block_number = 0;
    
    while (inst->data.opcode == OpLabel) {
        block.count = 0;
        struct instruction_list *start = inst->next;
        inst = inst->next; // NOTE: skip OpLabel
        
        while (!terminal(inst->data.opcode)) {
            inst = inst->next;
            ++block.count;
        }
        
        block.instructions = (block.count > 0 ? start : NULL);
        
        if (inst->data.opcode == OpBranch) {
            u32 edge_id = inst->data.OpBranch.target_label;
            u32 edge_index = search_item_u32(labels, bb_count, edge_id);
            cfg_add_edge(&file.cfg, block_number, edge_index);
        } else if (inst->data.opcode == OpBranchConditional) {
            // TODO: save condition res id for dump (probably save to cfg in a separate array)
            u32 true_edge = search_item_u32(labels, bb_count, inst->data.OpBranchConditional.true_label);
            u32 false_edge = search_item_u32(labels, bb_count, inst->data.OpBranchConditional.false_label);
            cfg_add_edge(&file.cfg, block_number, true_edge);
            cfg_add_edge(&file.cfg, block_number, false_edge);
        }
        
        struct instruction_list *save = inst;
        inst->prev->next = NULL;
        inst->next->prev = NULL;
        inst = inst->next;
        
        free(save);
        
        file.blocks[block_number++] = block;
    }
    
    //TODO: add_incoming_edges(&file.cfg);
    //TODO: file.cfg.dominators = cfgc_dominators(&file.cfg);
    
    // =======================
    
    
    // NOTE: all post-cfg instructions
    file.post_cfg = inst;
    // =======================
    
    return(file);
}

static void
dump_ir(struct ir *file, const char *filename)
{
    FILE *stream = fopen(filename, "wb");
    u32 buffer[100];
    
    if (!file) {
        fprintf(stderr, "[ERROR] Can not write output\n");
        exit(1);
    }
    
    fwrite(&file->header, sizeof(struct ir_header), 1, stream);
    
    struct instruction_list *inst = file->pre_cfg;
    u32 *words;
    
    do {
        words = dump_instruction(&inst->data, buffer);
        fwrite(words, inst->data.wordcount * 4, 1, stream);
        inst = inst->next;
    } while (inst);
    
    for (u32 i = 0; i < file->cfg.labels.size; ++i) {
        if (file->cfg.labels.data[i] == 0) {
            continue;
        }
        
        struct basic_block block = file->blocks[i];
        struct oplabel_t label_operand = {
            .result_id = file->cfg.labels.data[i]
        };
        
        struct instruction_t label_inst = {
            .opcode = OpLabel,
            .wordcount = 2,
            .OpLabel = label_operand
        };
        
        words = dump_instruction(&label_inst, buffer);
        fwrite(words, label_inst.wordcount * 4, 1, stream);
        
        inst = block.instructions;
        while (inst) {
            words = dump_instruction(&inst->data, buffer);
            fwrite(words, inst->data.wordcount * 4, 1, stream);
            inst = inst->next;
        };
        
        struct instruction_t termination_inst;
        struct edge_list *edge = file->cfg.out[i];
        u32 edge_count = 0;
        
        while (edge) {
            edge = edge->next;
            ++edge_count;
        }
        
        if (edge_count == 0) {
            termination_inst.opcode = OpReturn;
            termination_inst.wordcount = 1;
        } else if (edge_count == 1) {
            termination_inst.opcode = OpBranch;
            termination_inst.wordcount = 2;
            termination_inst.OpBranch.target_label = file->cfg.out[i]->data;
        } else if (edge_count == 2) {
            termination_inst.opcode = OpBranchConditional;
            termination_inst.wordcount = 3;
            termination_inst.OpBranchConditional.true_label = file->cfg.out[i]->data;
            termination_inst.OpBranchConditional.false_label = file->cfg.out[i]->next->data;
        } else {
            ASSERT(false);
        }
        
        words = dump_instruction(&termination_inst, buffer);
        fwrite(buffer, termination_inst.wordcount * 4, 1, stream);
    }
    
    inst = file->post_cfg;
    do {
        words = dump_instruction(&inst->data, buffer);
        fwrite(words, inst->data.wordcount * 4, 1, stream);
        inst = inst->next;
    } while (inst);
    
    fclose(stream);
}

// TODO: return status?
static void
delete_instruction(/*struct basic_block *block, */struct instruction_list **inst)
{
    struct instruction_list *vinst = *inst;
    
    if (vinst->prev) {
        vinst->prev->next = vinst->next;
    }
    
    if (vinst->next) {
        vinst->next->prev = vinst->prev;
    }
    
    *inst = NULL;
    free(vinst);
}

// TODO: return status?
static void
append_instruction(struct basic_block *block, struct instruction_list *inst)
{
    struct instruction_list *last = block->instructions;
    
    if (!last) {
        last = malloc(sizeof(struct instruction_list));
        last->data = inst->data;
        last->prev = NULL;
        last->next = NULL;
    }
    
    do {
        last = last->next;
    } while (last->next);
    
    last->next = malloc(sizeof(struct instruction_list));
    last->next->data = inst->data;
    last->next->prev = last;
    last->next->next = NULL;
}

static void
instruction_list_free(struct instruction_list *list)
{
    struct instruction_list *next;
    
    while (list) {
        next = list->next;
        free(list->data.unparsed_words);
        free(list);
        list = next;
    }
}

static void
destroy_ir(struct ir *file)
{
    for (u32 i = 0; i < file->cfg.labels.size; ++i) {
        instruction_list_free(file->blocks[i].instructions);
    }
    
    free(file->blocks);
    cfg_free(&file->cfg);
    instruction_list_free(file->pre_cfg);
    instruction_list_free(file->post_cfg);
}

static struct basic_block *
add_basic_block(struct ir *file)
{
    u32 label = file->header.bound;
    file->header.bound++;
    
    cfg_add_vertex(&file->cfg, label);
    
    struct basic_block new_block = {
        .count = 0,
        .instructions = NULL
    };
    
    file->blocks = realloc(file->blocks, file->cfg.labels.size * sizeof(struct basic_block));
    file->blocks[file->cfg.labels.size - 1] = new_block;
    
    return(file->blocks + file->cfg.labels.size - 1);
}

s32
main()
{
    const char *filename = "/home/aolo2/Desktop/vlk/data/cycle.frag.spv";
    
    u32 num_words;
    u32 *words = get_binary(filename, &num_words);
    
    struct ir file = eat_ir(words, num_words);
    free(words);
    
    //cfg_show(&file.cfg);
    //add_basic_block(&file);
    //cfg_show(&file.cfg);
    
    dump_ir(&file, "/home/aolo2/Desktop/vlk/data/cycle2.frag.spv");
    destroy_ir(&file);
}