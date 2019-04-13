#include "common.h"

#include "inst.c"
#include "cfg.c"
#include "ir.c"
#include "ssa.c"
#include "opt.c"

s32
main(void)
{
    u32 num_words;
    u32 *words = get_binary("data/cycle.frag.spv", &num_words);
    
    struct ir file = eat_ir(words, num_words);
    free(words);
    
    //struct cfg_dfs_result dfs = cfg_dfs(&file.cfg);
    //struct uint_vector df = ssa_dominance_frontier(&file.cfg, &dfs, ver, 2);
    ssa_convert(&file);
    
    dump_ir(&file, "data/cycle2.frag.spv");
    destroy_ir(&file);
    
    return(0);
}