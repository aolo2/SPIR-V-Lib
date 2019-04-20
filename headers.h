#include "common.h"

#include "inst.c"
#include "cfg.c"
#include "ir.c"
#include "ssa.c"

/*************************************************************/
/******************INSTRUCTION  MANIPULATION******************/
/*************************************************************/

/*

NOTE: one should refer themselves to the inst.h header, in which
 all supported instructions and their repspective structure are
 presented.
 
 If one whishes to expand the supported instructure set, the following
 steps are to be followed:
 
 1. Add the instruction opcode to the opcode_t enum
 2. Add a corresponding struct (recommened naming is xxx_t), where
 xxx is the new opcode. This struct should follow the binary SPIR-V
 layout as closely as possible
 3. Add a switch case in the 'instructon_parse' function, filling
 the new instruction structure using the word stream
 4. Add a switch case to the 'instruction_dump' function, filling
 a temporary buffer with the instructions operands. IMPORTANT: this
 step HAS TO follow the SPIR-V documentation so that the output
 file is a valid SPIR-V module.
 5. Modify any other helper functions which operate or switch on
 the opcode (such as the 'supported_in_cfg' function)
 
 */

/*************************************************************/
/**************END  OF INSTRUCTION  MANIPULATION**************/
/*************************************************************/



/*************************************************************/
/*****************IR  MANIPULATION  FUNCTIONS*****************/
/*************************************************************/

// NOTE: read a sequence of 4 byte words and produce an intermideate representation
struct ir 
ir_eat(u32 *data, u32 size);

// NOTE: dump the intermideate representation to a binary file
void 
ir_dump(struct ir *file, const char *filename);

// NOTE: delete instruction from (either from a basic block or from pre- or post-cfg)
void 
ir_delete_instruction(struct instruction_list **inst);

// NOTE: copy and insert the instruction before the first instruction of the given basic block
void 
ir_prepend_instruction(struct basic_block *block, struct instruction_t instruction);

// NOTE: copy and insert the instruction after the last instruction of the given basic block.
// If there are no instructions in the basic block, the passed intruction becomes the first one
void 
ir_append_instruction(struct basic_block *block, struct instruction_t instruction);

// to the created basic block. It's label can be read from TODO: the 'label' field
struct basic_block * 
ir_add_bb(struct ir *file);

// NOTE: free resources allocated by the intermideate represenation. After this procedure 
// the intermideate represenation can not be used
void 
ir_destroy(struct ir *file);

// NOTE: convert the IR to the Static Single Assignment form (as per Cytron E. et al)
void
ssa_convert(struct ir *file);

/*************************************************************/
/*************END  OF  IR  MANIPULATION  FUNCTIONS************/
/*************************************************************/


/*************************************************************/
/****************CFG  MANIPULATION  FUNCTIONS*****************/
/*************************************************************/

// NOTE: adds an outgoing edge at basic block 'from' to basic block 'to'
// as well as an incoming edge at basic block 'to' from basic block 'from'.
// Returns true if the action was succesful, and false otherwise
bool 
cfg_add_edge(struct ir_cfg *cfg, u32 from, u32 to);

// NOTE: removes an outgoing edge at basic block 'from' from basic block 'to'
// as well as an incoming edge at basic block 'to' from basic block 'from'.
// Returns true if the action was succesful, and false otherwise
bool
cfg_remove_edge(struct ir_cfg *cfg, u32 from, u32 to);

// NOTE: redirects all outgoing edges to basic block 'to_old' from basic block 'from'
// to basic block 'to_new'. Returns true if the action was succesful, and false otherwise
bool
cfg_redirect_edge(struct ir_cfg *cfg, u32 from, u32 to_old, u32 to_new);

// NOTE: removes the basic block with a given INDEX (not label) from the CFG, as well
// as all it's incoming and outgoing edges from all relevant basic blocks
void
cfg_remove_vertex(struct ir_cfg *cfg, u32 index);

// NOTE: debug display of the CFG
void
cfg_show(struct ir_cfg *cfg);

// NOTE: performs a Depth First Search on the CFG, and returns a structure, containing:
// u32 *preorder         - preorder[i] contains the preorder for the basic block with index i
// u32 *postorder        - postorder[i] contains the postorder for the basic block with index i
// u32 *parent           - parent[i] contains the parent in the DFS tree of basic block with index i
// u32 *sorted_preorder  - contains all reachable basic blocks sorted by preorder
// u32 *sorted_postorder - contains all reachable basic blocks sorted by postorder
// u32 size              - contains the number of traversed basic_blocks
struct cfg_dfs_result
cfg_dfs(struct ir_cfg *cfg);

// NOTE: performs a Breadth First Search on the CFG, and returns the order in which
// basic blocks have been traversed
struct uint_vector
cfg_bfs_order(struct ir_cfg *cfg);

// NOTE: performs a 'restricted' BFS, meaning that it starts at the given 'root'
// and also does not visit the 'terminate' block. Useful for traversing loops
// (one gives the loop header as 'root' and the loop merge block as 'terminate')
struct uint_vector
cfg_bfs_order_r(struct ir_cfg *cfg, u32 root, s32 terminate);

// NOTE: computes immediate dominators as per Lengauer-Tarjan, -1 means N/A,
// and is explicitly written to dominators[0]
s32 *
cfg_dominators(struct ir_cfg *input, struct cfg_dfs_result *dfs);

// NOTE: returns a non-negative number, which equals to the 'index'
// of the edge to basic block 'pred_index' in the 'block_index' block's
// incoming edge list
u32
cfg_whichpred(struct ir_cfg *cfg, u32 block_index, u32 pred_index);

/*************************************************************/
/************END  OF  CFG  MANIPULATION  FUNCTIONS************/
/*************************************************************/
