static bool
dominates(u32 parent, s32 child, s32 *dominators)
{
    if (parent == (u32) child) {
        return(true);
    }
    
    do {
        child = dominators[child];
        if (child == (s32) parent) {
            return(true);
        }
    } while (child != -1);
    
    return(false);
}

static void
optimize_lim()
{
    // for each opstore try to invariant in
    // while added continue
    
    // for each invariant move to new block
}