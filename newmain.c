bool
check_invariant(context, invariants, loop, location)
{
    pushed = false;
    
    switch (location.instruction.opcode) {
        case OpStore: {
            // NOTE: 
            
            pointer_id = get_operand(location.instruction, p_offset);
            store_location = find_producer(loop.result_ids, pointer_id);
            
            
            if (store_location == NULL) {
                pushed ||= pushed(invariants, location);
            } else {
                res &&= check_invariant(context, invariants, loop, store_location);
            }
            
        } break;
        
        case OpLoad: {
            pointer = get_operand(location, p_offset);
            
            // traverse dominators from this block
            for (other_location in stores_in_block(block)) {
                other_pointer = get_operand(other_location, p_offset);
                if (pointer == other_pointer) {
                    push(stack, other_location);
                }
            }
            
        } break;
        
        default: {
            // NOTE: arithmetics
            
            // check both operands, && the result
            
        }
    }
    
    return(pushed);
}

int
main(void)
{
    file = get_file();
    context = init_context(file);
    
    loops = get_loops(context);
    stores = get_stores(context);
    
    for (loop in loops) {
        
        invariants = [];
        
        while (changes) {
            changes = false;
            for (store in loop.stores) {
                if (add_invariant(context, invariants, loop, store)) {
                    changes = true;
                }
            }
        }
        
    }
    
    
    destroy_context(context);
}