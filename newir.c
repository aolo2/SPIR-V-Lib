#include "common.h"

struct ir_header {
    u32 magic_number;
    u32 version;
    u32 generator_mn;
    u32 bound;
    u32 zero;
};

struct instruction_list {
    struct instruction_t instruction;
    struct instrctuion_list *next;
    struct instrctuion_list *prev;
};

struct basic_block {
    u32 size;
    struct instruction_list *instructions;
};

struct ir_cfg {
    u32 nver;
    u32 neds;
    // TODO: cfg
    s32 *dominators;
};

struct ir {
    struct ir_header header;
    struct ir_cfg cfg;
};