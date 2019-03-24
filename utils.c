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

static inline void
show_array_u32(u32 *array, u32 size, const char *header)
{
    printf("%s:", header);
    for (u32 i = 0; i < size; ++i) {
        printf(" %d", array[i]);
    }
    printf("\n");
}

static inline void
show_array_s32(s32 *array, u32 size, const char *header)
{
    printf("%s:", header);
    for (u32 i = 0; i < size; ++i) {
        printf(" %d", array[i]);
    }
    printf("\n");
}

static inline void
show_vector_u32(struct uint_vector *v, const char *header)
{
    show_array_u32(v->data, v->size, header);
}

static void
show_cfg(struct ir *file, struct cfgc *cfg, u32 *bb_labels, u32 num_bb)
{
    for (u32 i = 0; i < num_bb; ++i) {
        printf("=== Basic block %d ===\n", file->blocks[i].id);
        for (u32 j = file->blocks[i].from; j < file->blocks[i].to; ++j) {
            printf("%s\n", names_opcode(file->instructions[j].opcode));
        }
        
        for (u32 j = cfg->edges_from.data[i]; j < cfg->edges_from.data[i + 1]; ++j) {
            printf("edge to -> %d\n", bb_labels[cfg->edges.data[j]]);
        }
        
        for (u32 j = cfg->in_edges_from.data[i]; j < cfg->in_edges_from.data[i + 1]; ++j) {
            printf("edge from <- %d\n", bb_labels[cfg->in_edges.data[j]]);
        }
        
        printf("\n");
    }
    
    //show_array_u32(bb_labels, num_bb, "Labels");
    //show_array_s32(dominators, cfg.nver, "Dominators");
}