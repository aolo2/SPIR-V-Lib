#include "headers.h"

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

s32
main(void)
{
    u32 num_words;
    u32 *words = get_binary("data/sample.frag.spv", &num_words);
    struct ir file = ir_eat(words, num_words);
    
    // NOTE: for any OpVariable there is only ONE OpStore (and maybe one OpLoad)
    ssa_convert(&file);
    
    ir_dump(&file, "data/sample.frag.spv.opt");
    ir_destroy(&file);
    
    return(0);
}
