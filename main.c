#include "main.h"

static void
add_incoming_edges(struct cfgc *cfg)
{
    struct uint_vector *edges_growing = malloc(cfg->nver * sizeof(struct uint_vector));
    for (u32 i = 0; i < cfg->nver; ++i) {
        edges_growing[i] = vector_init();
    }
    
    // NOTE: run through outgoind edges and grow the
    // respective incoming lists
    for (u32 i = 0; i < cfg->nver; ++i) {
        for (u32 j = cfg->edges_from.data[i]; j <  cfg->edges_from.data[i + 1]; ++j) {
            u32 edge_from = i;
            u32 edge_to = cfg->edges.data[j];
            vector_push(edges_growing + edge_to, edge_from);
        }
    }
    
    cfg->in_edges      = vector_init_sized(cfg->neds);
    cfg->in_edges_from = vector_init_sized(cfg->nver + 1);
    
    u32 total_edges = 0;
    for (u32 i = 0; i < cfg->nver; ++i) {
        vector_push(&cfg->in_edges_from, total_edges);
        for (u32 j = 0; j < edges_growing[i].size; ++j) {
            vector_push(&cfg->in_edges, edges_growing[i].data[j]);
        }
        total_edges += edges_growing[i].size;
    }
    
    for (u32 i = 0; i < cfg->nver; ++i) {
        vector_free(edges_growing + i);
    }
    
    // NOTE: additional element for easier indexing
    vector_push(&cfg->in_edges_from, cfg->neds);
    
    free(edges_growing);
}

s32
main(void)
{
    u32 num_words, num_bb, num_instructions;
    
    u32 *words = get_binary("/home/aolo2/Documents/code/study/vlk/data/cycle.frag.spv", &num_words);
    struct parse_result parsed = parse_words(words, num_words, &num_instructions, &num_bb);
    // struct spirv_header header = *(struct spirv_header *) words;
    
    struct ir file = {
        .constants = vector_init(),
        .words = words,
        .instructions = parsed.instructions,
        .blocks = malloc(num_bb * sizeof(struct basic_block))
    };
    
    struct cfgc cfg = construct_cfg(&file, parsed.bb_labels, num_instructions, num_bb);
    s32 *dominators = cfgc_dominators(&cfg);
    
    show_cfg(&file, &cfg, parsed.bb_labels, num_bb);
    
    u32 loop_count = 0;
    struct uint_vector *loops = malloc(num_bb * sizeof(struct uint_vector));
    
    // NOTE: scout for constants
    for (u32 i = 0; i < num_instructions; ++i) {
        if (is_constant_insruction(file.instructions[i].opcode)) {
            vector_push(&file.constants, file.words[file.instructions[i].words_from + CONSTANTS_RESID_INDEX]);
        }
        
        if (file.instructions[i].opcode == OpLabel) {
            break;
        }
    }
    
    // NOTE: scout for OpLoopMerge
    for (u32 i = 0; i < num_bb; ++i) {
        for (u32 j = file.blocks[i].from; j < file.blocks[i].to; ++j) {
            if (file.instructions[j].opcode == OpLoopMerge) {
                u32 merge_block = file.words[file.instructions[j].words_from + OPLOOPMERGE_MERGE_INDEX];
                u32 bb_index = search_item_u32(parsed.bb_labels, num_bb, merge_block);
                loops[loop_count++] = cfgc_bfs_restricted(&cfg, i, bb_index);
            }
        }
    }
    
    for (u32 i = 0; i < loop_count; ++i) {
        optimize_lim(loops + i, &file);
    }
    
    
    // NOTE: free
    for (u32 i = 0; i < loop_count; ++i) {
        vector_free(loops + i);
    }
    
    free(file.words);
    free(file.instructions);
    free(file.blocks);
    free(parsed.bb_labels);
    
    cfgc_free(&cfg);
    
    return(0);
}