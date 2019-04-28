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

struct ir
ir_eat(u32 *data, u32 size)
{
    u32 labels[100] = { 0 };
    u32 bb_count = 0;
    
    struct ir file;
    file.header = *((struct ir_header *) data);
    
    struct instruction_list *all_instructions = NULL;
    struct instruction_list *inst = NULL;
    struct instruction_list *last = NULL;
    
    u32 offset = sizeof(struct ir_header) / 4;
    u32 *word = data + offset;
    
    // NOTE: get all instructions in a list, count basic blocks
    do {
        inst = malloc(sizeof(struct instruction_list));
        inst->data = instruction_parse(word);
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
            file.cfg.conditions[block_number] = inst->data.OpBranchConditional.condition;
            u32 true_edge = search_item_u32(labels, bb_count, inst->data.OpBranchConditional.true_label);
            u32 false_edge = search_item_u32(labels, bb_count, inst->data.OpBranchConditional.false_label);
            cfg_add_edge(&file.cfg, block_number, true_edge);
            cfg_add_edge(&file.cfg, block_number, false_edge);
        }
        
        if (!supported_in_cfg(inst->data.opcode)) {
            fprintf(stderr, "[ERROR] Unsupported instruction (opcode %d) in CFG\n", inst->data.opcode);
            exit(1);
        }
        
        struct instruction_list *save = inst;
        inst->prev->next = NULL;
        inst->next->prev = NULL;
        inst = inst->next;
        
        free(save);
        
        file.blocks[block_number++] = block;
    }
    
    struct cfg_dfs_result dfs = cfg_dfs(&file.cfg);
    file.cfg.dominators = cfg_dominators(&file.cfg, &dfs);
    // =======================
    
    
    // NOTE: all post-cfg instructions
    file.post_cfg = inst;
    // =======================
    
    return(file);
}

void
ir_dump(struct ir *file, const char *filename)
{
    FILE *stream = fopen(filename, "wb");
    u32 buffer[256];
    
    if (!file) {
        fprintf(stderr, "[ERROR] Can not write output\n");
        exit(1);
    }
    
    fwrite(&file->header, sizeof(struct ir_header), 1, stream);
    
    struct instruction_list *inst = file->pre_cfg;
    u32 *words;
    
    do {
        words = instruction_dump(&inst->data, buffer);
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
        
        words = instruction_dump(&label_inst, buffer);
        fwrite(words, label_inst.wordcount * 4, 1, stream);
        
        inst = block.instructions;
        while (inst) {
            words = instruction_dump(&inst->data, buffer);
            fwrite(words, inst->data.wordcount * 4, 1, stream);
            inst = inst->next;
        }
        
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
            termination_inst.OpBranch.target_label = file->cfg.labels.data[file->cfg.out[i]->data];
        } else if (edge_count == 2) {
            termination_inst.opcode = OpBranchConditional;
            termination_inst.wordcount = 4;
            termination_inst.OpBranchConditional.condition = file->cfg.conditions[i];
            termination_inst.OpBranchConditional.true_label =
                file->cfg.labels.data[file->cfg.out[i]->data];
            termination_inst.OpBranchConditional.false_label =
                file->cfg.labels.data[file->cfg.out[i]->next->data];
        } else {
            ASSERT(false);
        }
        
        words = instruction_dump(&termination_inst, buffer);
        fwrite(buffer, termination_inst.wordcount * 4, 1, stream);
    }
    
    inst = file->post_cfg;
    do {
        words = instruction_dump(&inst->data, buffer);
        fwrite(words, inst->data.wordcount * 4, 1, stream);
        inst = inst->next;
    } while (inst);
    
    fclose(stream);
}

void
ir_delete_instruction(struct basic_block *block, struct instruction_list *inst)
{
    if (inst->prev) {
        inst->prev->next = inst->next;
    }
    
    if (inst->next) {
        inst->next->prev = inst->prev;
    }
    
    if (block->instructions == inst) {
        block->instructions = inst->next;
    }
    
    free(inst);
    
    block->count -= 1;
}

void
ir_prepend_instruction(struct basic_block *block, struct instruction_t instruction)
{
    struct instruction_list *first = block->instructions;
    
    if (!first) {
        first = malloc(sizeof(struct instruction_list));
        first->data = instruction;
        first->prev = NULL;
        first->next = NULL;
        block->instructions = first;
    } else {
        block->instructions = malloc(sizeof(struct instruction_list));
        block->instructions->data = instruction;
        block->instructions->prev = NULL;
        block->instructions->next = first;
        first->prev = block->instructions;
    }
    
    block->count++;
}

void
ir_append_instruction(struct basic_block *block, struct instruction_t instruction)
{
    struct instruction_list *last = block->instructions;
    
    if (!last) {
        last = malloc(sizeof(struct instruction_list));
        last->data = instruction;
        last->prev = NULL;
        last->next = NULL;
        block->instructions = last;
    } else {
        while (last->next) {
            last = last->next;
        }
        
        last->next = malloc(sizeof(struct instruction_list));
        last->next->data = instruction;
        last->next->prev = last;
        last->next->next = NULL;
    }
    
    block->count++;
}

static void
instruction_list_free(struct instruction_list *list)
{
    struct instruction_list *next;
    
    while (list) {
        next = list->next;
        
#if 0
        // TODO: clean up instruction destuctors
        if (list->data.opcode == OpName) {
            free(list->data.OpName.name);
        }
#endif
        
        if (!supported_in_cfg(list->data.opcode)) { 
            free(list->data.unparsed_words);
        }
        
        free(list);
        list = next;
    }
}

void
ir_destroy(struct ir *file)
{
    for (u32 i = 0; i < file->cfg.labels.size; ++i) {
        instruction_list_free(file->blocks[i].instructions);
    }
    
    free(file->blocks);
    cfg_free(&file->cfg);
    instruction_list_free(file->pre_cfg);
    instruction_list_free(file->post_cfg);
}

u32
ir_add_bb(struct ir *file)
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
    
    return(file->cfg.labels.size - 1);
}

void
ir_add_opname(struct ir *file, u32 target_id, char *name)
{
    struct instruction_list *inst = file->pre_cfg;
    while (inst) {
        if (inst->data.opcode == OpExecutionMode) {
            u32 next_opcode = inst->next->data.opcode;
            
            while (next_opcode >= OpSourceContinued && next_opcode <= OpName) {
                inst = inst->next;
                next_opcode = inst->next->data.opcode;
            }
            
            struct instruction_list *newinst = malloc(sizeof(struct instruction_list));
            
            newinst->next = inst->next;
            newinst->prev = inst;
            
            if (inst->next) {
                inst->next->prev = newinst;
            }
            
            newinst->data.opcode = OpName;
            newinst->data.unparsed_words = NULL;
            newinst->data.OpName.target_id = target_id;
            newinst->data.OpName.name = strdup(name);
            
            u32 literal_len = (u32) strlen(name) + 1;
            newinst->data.wordcount = 2 + literal_len / 4 + 1;
            
            inst->next = newinst;
        }
        
        inst = inst->next;
    }
}

void 
ir_delete_opname(struct ir *file, u32 target_id)
{
    struct instruction_list *inst = file->pre_cfg;
    while (inst) {
        if (inst->data.opcode == OpName) {
            if (inst->data.OpName.target_id == target_id) {
                // NOTE: we know that OpName can not be the first instrction
                inst->prev->next = inst->next;
                if (inst->next) {
                    inst->next->prev = inst->prev;
                }
                free(inst);
                break;
            }
        }
        inst = inst->next;
    }
}