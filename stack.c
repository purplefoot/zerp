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
    *(++zSP) = value;
    LOG(ZDEBUG, "\nPush %04x", value); 
    return value;
}

zword_t stack_pop() {
    zword_t value;

    if (zSP == zFP->sp) {
        fatal_error("Value stack underflow!\n");
    }
    value =  *(zSP--);
    LOG(ZDEBUG, "\nPop %04x", value); 
    return value;
}

/*
    In the seven opcodes that take indirect variable references (inc, dec,
    inc_chk, dec_chk, load, store, pull), an indirect reference to the stack
    pointer does not push or pull the top item of the stack - it is read
    or written in place.

    We will call this peeking and poking the stack (because 80s retro is in, dude.)
*/
zword_t stack_peek() {
    LOG(ZDEBUG, "\nPeek %04x", *(zSP));
    return *(zSP);
}

zword_t stack_poke(zword_t value) {
    return *(zSP) = value;
}

zstack_frame_t * call_zroutine(packed_addr_t address, zoperand_t *operands, zbyte_t ret_store){
    zbyte_t local_count;
    zstack_frame_t *newFrame;
    int i = 0;
    
    if (address == 0) {
        variable_set(ret_store, 0);
        return;
    }
    
    newFrame = zFP +1;
    if (newFrame > zCallStackTop)
        fatal_error("Call stack overflow");
    
    newFrame->pc = zPC;
    newFrame->sp = zSP;
    newFrame->ret_store = ret_store;
    
	local_count = get_byte(address++);
    for (i = 0; i < local_count; i++) {
        newFrame->locals[i] = get_word(address);
        address += 2;   
    }
	while (i++ < 16)
		newFrame->locals[i] = 0;
    
	i = 0;
    while (operands->type != NONE) {
        newFrame->locals[i++] = operands->type == VARIABLE ? variable_get(operands->bytes) : operands->bytes;
        operands++;
    }
    
    zPC = address;
    zSP++;
    
    return zFP++;
}

zstack_frame_t *return_zroutine(zword_t ret_value) {
    zbyte_t ret_store;
    
    if ((zFP - 1) < zCallStack)
        fatal_error("Call stack underflow");
    
    zSP = zFP->sp;
    zPC = zFP->pc;
    ret_store = zFP->ret_store;
    
    zFP--;
    
    variable_set(ret_store, ret_value);
    
    return zFP;
}
