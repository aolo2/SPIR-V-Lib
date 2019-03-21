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
    
    //struct spirv_header header = *(struct spirv_header *) words;
    struct parse_result parsed = parse_words(words, num_words, &num_instructions, &num_bb);
    struct instruction *instructions = parsed.instructions;
    struct basic_block *blocks = malloc(num_bb * sizeof(struct basic_block));
    u32 *bb_labels = parsed.bb_labels;
    
    struct cfgc cfg = construct_cfg(words,
                                    instructions, 
                                    blocks, 
                                    bb_labels, 
                                    num_instructions, 
                                    num_bb);
    
    add_incoming_edges(&cfg);
    s32 *dominators = cfgc_dominators(&cfg);
    
    struct uint_vector constants = vector_init();
    struct bfs_result *loops = malloc(num_bb * sizeof(struct bfs_result));
    u32 loop_count = 0;
    
    // NOTE: scout for OpLoopMerge to find cycles
    for (u32 i = 0; i < num_bb; ++i) {
        for (u32 j = blocks[i].from; j < blocks[i].to; ++j) {
            if (is_constant_insruction(instructions[j].opcode)) {
                vector_push(&constants, words[instructions[j].words_from + CONSTANTS_RESID_INDEX]);
            }
            
            if (instructions[j].opcode == OpLoopMerge) {
                u32 merge_block = words[instructions[j].words_from + OPLOOPMERGE_MERGE_INDEX];
                u32 bb_index = search_item_u32(bb_labels, num_bb, merge_block);
                loops[loop_count++] = cfgc_bfs_restricted(&cfg, i, bb_index, blocks, instructions, words);
            }
        }
    }
    
    // NOTE: scan to find all contants
    for (u32 i = 0; i < num_instructions; ++i) {
        if (is_constant_insruction(instructions[i].opcode)) {
            vector_push(&constants, words[instructions[i].words_from + CONSTANTS_RESID_INDEX]);
        } else if (instructions[i].opcode == OpLabel) {
            // we've seen enough
            break;
        }
    }
    
    for (u32 i = 0; i < loop_count; ++i) {
        optimize_lim(loops + i, &constants, blocks, instructions, words);
    }
    
#if 0
    for (u32 i = 0; i < num_bb; ++i) {
        printf("=== Basic block %d ===\n", blocks[i].id);
        for (u32 j = blocks[i].from; j < blocks[i].to; ++j) {
            printf("%s\n", names_opcode(instructions[j].opcode));
        }
        
        for (u32 j = cfg.edges_from.data[i]; j < cfg.edges_from.data[i + 1]; ++j) {
            printf("edge to -> %d\n", bb_labels[cfg.edges.data[j]]);
        }
        
        for (u32 j = cfg.in_edges_from.data[i]; j < cfg.in_edges_from.data[i + 1]; ++j) {
            printf("edge from <- %d\n", bb_labels[cfg.in_edges.data[j]]);
        }
        
        printf("\n");
    }
    
    show_array_u32(bb_labels, num_bb, "Labels");
    show_array_s32(dominators, cfg.nver, "Dominators");
#endif
    
    for (u32 i = 0; i < loop_count; ++i) {
        bfs_result_free(loops + i);
    }
    
    free(words);
    free(bb_labels);
    free(instructions);
    free(blocks);
    
    cfgc_free(&cfg);
    
    return(0);
}