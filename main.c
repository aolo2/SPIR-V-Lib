#include "main.h"

s32
main(void)
{
    const char *filename = "C:/Users/Alex/code/vlk/data/cycle.frag.spv"; // TODO: argv[1]
    const char *filename_out = "C:/Users/Alex/code/vlk/data/cycle.frag.spv.opt";
    
    u32 num_words, num_bb, num_instructions;
    u32 *words = get_binary(filename, &num_words);
    
    struct parse_result parsed = parse_words(words, num_words, &num_instructions, &num_bb);
    struct spirv_header header = *(struct spirv_header *) words;
    
    struct ir file = {
        .header = header,
        .words = words,
        .instructions = parsed.instructions,
        .blocks = malloc(num_bb * sizeof(struct basic_block)),
        .bb_labels = parsed.bb_labels,
        .num_words = num_words,
        .num_instructions = num_instructions,
        .num_bb = num_bb
    };
    
    file.cfg = construct_cfg(&file, parsed.bb_labels, num_instructions, num_bb);
    file.dominators = cfgc_dominators(&file.cfg);
    
    u32 loop_count = 0;
    struct uint_vector *loops_bfs = malloc(num_bb * sizeof(struct uint_vector));
    
    // NOTE: scout for OpLoopMerge
    for (u32 i = 0; i < num_bb; ++i) {
        for (u32 j = file.blocks[i].from; j < file.blocks[i].to; ++j) {
            if (file.instructions[j].opcode == OpLoopMerge) {
                u32 merge_block = file.words[file.instructions[j].words_from + OPLOOPMERGE_MERGE_INDEX];
                u32 bb_index = search_item_u32(parsed.bb_labels, num_bb, merge_block);
                
                loops_bfs[loop_count] = cfgc_bfs_restricted(&file.cfg, i, bb_index);
                ++loop_count;
            }
        }
    }
    
    for (u32 i = 0; i < loop_count; ++i) {
        optimize_lim(loops_bfs + i, &file);
    }
    
    dump_ir(&file, filename_out);
    
    // NOTE: free
    for (u32 i = 0; i < loop_count; ++i) {
        vector_free(loops_bfs + i);
    }
    
    free(file.words);
    free(file.instructions);
    free(file.blocks);
    free(parsed.bb_labels);
    
    cfgc_free(&file.cfg);
    
    return(0);
}