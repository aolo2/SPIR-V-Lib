#include "common.h"

#include "inst.c"
#include "cfg.c"
#include "ssa.c"
#include "opt.c"
#include "ir.c"

s32
main(void)
{
    u32 num_words;
    u32 *words = get_binary("data/ssa.frag.spv", &num_words);
    
    struct ir file = eat_ir(words, num_words);
    free(words);
    
    u32 ver[] = { 0, 1, 2 };
    
    struct cfg_dfs_result dfs = cfg_dfs(&file.cfg);
    struct uint_vector df = ssa_dominance_frontier(&file.cfg, &dfs, ver, 3);
    
    dump_ir(&file, "data/ssa2.frag.spv");
    destroy_ir(&file);
    
    return(0);
}