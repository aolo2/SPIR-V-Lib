static bool
is_termination_instruction(u32 opcode)
{
    return(opcode >= OpBranch && opcode <= OpUnreachable);
}

// NOTE: res_id producers we support
static bool
is_result_id_producer(u32 opcode)
{
    return(opcode == OpVariable || opcode == OpLoad || 
           (opcode >= OpSNegate && opcode <= OpFMod));
}

static bool
is_constant_insruction(u32 opcode)
{
    return(opcode == OpConstantTrue || opcode == OpConstantFalse ||
           opcode == OpConstant || opcode == OpConstantNull);
}

static struct parse_result
parse_words(u32 *words, u32 num_words, u32 *num_instructions, u32 *num_bb)
{
    struct parse_result res = {
        .instructions = malloc(1000 * sizeof(struct instruction)),
        .bb_labels    = malloc(100 * sizeof(u32))
    };
    
    *num_instructions = 0;
    *num_bb = 0;
    
    for (u32 i = SPIRV_HEADER_WORDS; i < num_words; ++i) {
        struct instruction inst = {
            .wordcount = (words[i] & WORDCOUNT_MASK) >> 16,
            .opcode = words[i] & OPCODE_MASK,
            .words_from = i
        };
        
        if (inst.opcode == OpLabel) {
            res.bb_labels[(*num_bb)++] = *(words + i + OPLABEL_RESID_INDEX);
        }
        
        res.instructions[(*num_instructions)++] = inst;
        i += inst.wordcount - 1;
    }
    
    return(res);
}

static struct cfgc
construct_cfg(struct ir *file, u32 *bb_labels, u32 num_instructions, u32 num_bb)
{
    struct cfgc cfg = {
        .nver           = 0,
        .neds           = 0,
        .edges_from     = vector_init(),
        .edges          = vector_init()
    };
    
    u32 bb_head = 0;
    
    for (u32 i = 0; i < num_instructions; ++i) {
        struct instruction *inst = file->instructions + i;
        struct basic_block bb;
        
        if (inst->opcode == OpLabel) {
            // NOTE: beginning of a Basic Block
            bb.from = i;
            bb.id = file->words[inst->words_from + OPLABEL_RESID_INDEX];
            
            ++cfg.nver;
            vector_push(&cfg.edges_from, cfg.neds);
            
            //u32 merge_block    = upper_bound;
            //u32 continue_block = upper_bound;
            
            do {
                ++i;
                inst = file->instructions + i;
            } while (!is_termination_instruction(inst->opcode));
            
            bb.to = i + 1;
            file->blocks[bb_head++] = bb;
            
            switch (inst->opcode) {
                case OpReturn:
                case OpReturnValue: {
                } break;
                
                case OpBranchConditional: {
                    //u32 condition  = words[inst.words_from + OPBRANCHCOND_COND_INDEX];
                    u32 t_label    = file->words[inst->words_from + OPBRANCHCOND_TLABEL_INDEX];
                    u32 f_label    = file->words[inst->words_from + OPBRANCHCOND_FLABEL_INDEX];
                    
                    s32 bb_index = search_item_u32(bb_labels, num_bb, t_label);
                    ++cfg.neds;
                    vector_push(&cfg.edges, bb_index);
                    
                    bb_index = search_item_u32(bb_labels, num_bb, f_label);
                    ++cfg.neds;
                    vector_push(&cfg.edges, bb_index);
                } break;
                
                case OpBranch: {
                    u32 target_label = file->words[inst->words_from + OPBRANCH_TARGETLABEL_INDEX];
                    s32 bb_index = search_item_u32(bb_labels, num_bb, target_label);
                    
                    ASSERT(bb_index != -1);
                    
                    ++cfg.neds;
                    vector_push(&cfg.edges, bb_index);
                } break;
                
                default: {  
                    NOTIMPLEMENTED;
                }
            }
        }
    }
    
    // NOTE: additional element for easier indexing
    vector_push(&cfg.edges_from, cfg.neds);
    
    ASSERT(bb_head == num_bb);
    
    add_incoming_edges(&cfg);
    
    return(cfg);
}

// NOTE: this only works for a SMALL SUBSET of instructions
static u32
extract_res_id(struct instruction *inst, u32 *words)
{
    switch (inst->opcode) {
        case OpVariable: {
            return(words[inst->words_from + OPVARIABLE_RESID_INDEX]);
        } break;
        
        case OpLoad: {
            return(words[inst->words_from + OPLOAD_RESID_INDEX]);
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
            return(words[inst->words_from + ARITHMETIC_OPS_RESID_INDEX]);
        } break;
    }
    
    return(0);
}

static u32
extract_operand_count(struct instruction *inst)
{
    switch (inst->opcode) {
        case OpLoad:
        case OpSNegate:
        case OpFNegate: {
            return(1);
        } break;
        
        case OpStore:
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
            return(2);
        } break;
    }
    
    fprintf(stderr, "%s\n", names_opcode(inst->opcode));
    
    SHOULDNOTHAPPEN;
}

static u32
extract_nth_operand(struct instruction *inst, u32 *words, u32 n)
{
    switch (inst->opcode) {
        case OpLoad: {
            ASSERT(n == 0);
            return(words[inst->words_from + OPLOAD_POINTER_INDEX]);
        } break;
        
        case OpStore: {
            ASSERT(n == 0 || n == 1);
            return(words[inst->words_from + OPSTORE_OPS_OPERANDS_START_INDEX + n]);
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
            ASSERT(n == 0 || n == 1);
            return(words[inst->words_from + ARITHMETIC_OPS_OPERANDS_START_INDEX + n]);
        }
    }
    
    SHOULDNOTHAPPEN;
}

static bool
matters_in_cycle(u32 opcode)
{
    switch (opcode) {
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
        case OpFMod: {
            return(true);
        }
        
        default: {
            return(false);
        }
    }
}

static void
dump_ir(struct ir *file, const char *filename)
{
    FILE *stream = fopen(filename, "wb");
    
    if (!file) {
        fprintf(stderr, "[ERROR] Can not write output\n");
        exit(1);
    }
    
    fwrite(&file->header, SPIRV_HEADER_WORDS * WORD_SIZE, 1, stream);
    fwrite(file->words + SPIRV_HEADER_WORDS, 
           (file->num_words - SPIRV_HEADER_WORDS) * WORD_SIZE, 1, stream);
    
    fclose(stream);
}