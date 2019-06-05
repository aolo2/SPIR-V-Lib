#include "headers.h"

#include "opt.c"

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
main(s32 argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "[ERROR] Usage: ./%s in out", argv[0]);
        return(1);
    }
    
    u32 num_words;
    u32 *words = get_binary(argv[1], &num_words);
    ASSERT(words);
    
    struct ir file = ir_eat(words, num_words);
    
    ssa_convert(&file);
    loop_invariant_code_motion(&file);
    
    ir_dump(&file, argv[2]);
    ir_destroy(&file);
    
    return(0);
}
