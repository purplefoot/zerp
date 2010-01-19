/*
    Zerp: a Z-machine interpreter
    stack.c : z-machine stacks - value and call
*/

#include "glk.h"
#include "zerp.h"
#include "stack.h"

#if DEBUG > ZDEBUG
static void dump_stack() {
    zword_t *s;

    glk_put_string("stack : ");
    
    s = zSP;
    while(--s >= zStack)
        glk_printf("%8x", *s);
        
    glk_put_string("\n");
}
#endif

int stack_push(zword_t value) {
#if DEBUG > ZDEBUG
    dump_stack();
#endif
    *(zSP++) = value;
    if (zSP >= zStackTop) {
        fatal_error("Value stack overflow!\n");
    }
}

zword_t stack_pop() {
#if DEBUG > ZDEBUG
    dump_stack();
#endif
    if (zSP == zFP->sp) {
        // BUG: throw an error here (once all stack ops are working)
        return 0;
    }
    return *(--zSP);
}

zstack_frame_t * call_zroutine(packed_addr_t address, zword_t *operands, int opcount, zbyte_t ret_value){
    zbyte_t local_count;
    int i;
    
    if (address == 0) {
        variable_set(ret_value, 0);
        return;
    }
    
    if (zFP++ > zCallStackTop)
        fatal_error("Call stack overflow");
    
    zFP->pc = zPC;
    zFP->sp = zSP;
    zFP->ret_value = ret_value;
    
    for (local_count = get_byte(address++); local_count > 0; local_count--) {
        zFP->locals[local_count - 1] = get_word(address);
        address += 2;        
    }
    
    for (i = 0; i < opcount; i++)
        zFP->locals[i] = operands[i];
    
    zPC = address;
    
    return zFP;
}

zstack_frame_t *return_zroutine(zword_t ret_value) {
    if ((zFP - 1) < zCallStack)
        fatal_error("Call stack underflow");
    
    zSP = zFP->sp;
    zPC = zFP->pc;
    
    variable_set(zFP->ret_value, ret_value);
    
    return zFP--;
}
