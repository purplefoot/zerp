/*
    Zerp: a Z-machine interpreter
    stack.c : z-machine stacks - value and call
*/

#include "glk.h"
#include "zerp.h"
#include "opcodes.h"
#include "stack.h"

int stack_push(zword_t value) {
    if (zSP >= zStackTop) {
        fatal_error("Value stack overflow!\n");
    }
    *(zSP++) = value;
    LOG(ZDEBUG, "\nPush %04x", value); 
    return value;
}

zword_t stack_pop() {
    zword_t value;
    if (zSP == zFP->sp) {
        fatal_error("Value stack underflow!\n");
    }
    value =  *(--zSP);
    LOG(ZDEBUG, "\nPop %04x", value); 
    return value;
}

zstack_frame_t * call_zroutine(packed_addr_t address, zoperand_t *operands, zbyte_t ret_value){
    zbyte_t local_count;
    int i = 0;
    
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
    
    while (operands->type != NONE)
        zFP->locals[i++] = operands++->bytes;
    
    zPC = address;
    
    return zFP;
}

zstack_frame_t *return_zroutine(zword_t ret_value) {
    if ((zFP - 1) < zCallStack)
        fatal_error("Call stack underflow");
    
    zSP = zFP->sp;
    zPC = zFP->pc;
    
    zFP--;
    
    variable_set(zFP->ret_value, ret_value);
    
    return zFP;
}
