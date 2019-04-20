static const u32 WORDCOUNT_MASK     = 0xFFFF0000;
static const u32 OPCODE_MASK        = 0x0000FFFF;

enum opcode_t {
    OpVariable = 59,
    OpLoad = 61,
    OpStore = 62,
    OpSNegate = 126,
    OpFNegate = 127,
    OpIAdd = 128,
    OpFAdd = 129,
    OpISub = 130,
    OpFSub = 131,
    OpIMul = 132,
    OpFMul = 133,
    OpUDiv = 134,
    OpSDiv = 135,
    OpFDiv = 136,
    OpUMod = 137,
    OpSRem = 138,
    OpSMod = 139,
    OpFRem = 140,
    OpFMod = 141,
    OpPhi = 245,
    OpLoopMerge = 246,
    OpSelectionMerge = 247,
    OpLabel = 248,
    OpBranch = 249,
    OpBranchConditional = 250,
    OpReturn = 253,
};

struct oplabel_t {
    u32 result_id;
};

struct opvariable_t {
    u32 result_type;
    u32 result_id;
    u32 storage_class;
    u32 initializer; // optional
};

struct opbranch_t {
    u32 target_label;
};

struct opbranchconditional_t {
    u32 condition;
    u32 true_label;
    u32 false_label;
    // does not support label weights
};

struct opselectionmerge_t {
    u32 merge_block;
    u32 selection_control;
};

struct opphi_t {
    u32 result_type;
    u32 result_id;
    u32 *variables;
    u32 *parents;
};

struct oploopmerge_t {
    u32 merge_block;
    u32 continue_block;
    u32 loop_control;
};

struct opstore_t {
    u32 pointer;
    u32 object;
    u32 memory_access; // optional
};

struct opload_t {
    u32 result_type;
    u32 result_id;
    u32 pointer;
    u32 memory_access; // optional
};

struct unary_arithmetics_layout {
    u32 result_type;
    u32 result_id;
    u32 operand;
};

struct binary_arithmetics_layout {
    u32 result_type;
    u32 result_id;
    u32 operand_1;
    u32 operand_2;
};

struct instruction_t {
    enum opcode_t opcode;
    u32 wordcount;
    u32 *unparsed_words;
    
    union {
        struct oplabel_t OpLabel;
        struct opvariable_t OpVariable;
        struct opbranch_t OpBranch;
        struct opbranchconditional_t OpBranchConditional;
        struct opphi_t OpPhi;
        struct opselectionmerge_t OpSelectionMerge;
        struct oploopmerge_t OpLoopMerge;
        struct opstore_t OpStore;
        struct opload_t OpLoad;
        struct unary_arithmetics_layout unary_arithmetics;
        struct binary_arithmetics_layout binary_arithmetics;
    };
};
