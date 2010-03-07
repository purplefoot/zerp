/*
    Zerp: a Z-machine interpreter
    stack.c : z-machine stacks - value and call
*/

#include <stdio.h>
#ifdef TARGET_OS_MAC
#include <GlkClient/glk.h>
#else
#include "glk.h"
#endif /* TARGET_OS_MAC */
#include "zerp.h"
#include "opcodes.h"
#include "stack.h"

int stack_push(zword_t value) {
    if (zSP >= zStackTop) {
        fatal_error("Value stack overflow!\n");
    }
    *(++zSP) = value;
    LOG(ZDEBUG, "\nStack: push - size now %li, frame usage %li (pushed #%x)", 2051 - (zSP - zStack), (zSP - zFP->sp) - 1, value); 
    return value;
}

zword_t stack_pop() {
    zword_t value;

    if (zSP == zFP->sp) {
        fatal_error("Value stack underflow!\n");
    }
    value =  *(zSP--);
    LOG(ZDEBUG, "\nStack: pop - size now %li, frame usage %li (value #%x)", 2051 - (zSP - zStack), (zSP - zFP->sp) - 1, value); 
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
    LOG(ZDEBUG, "\nStack: pop - size now %li, frame usage %li (pushed #%x)", 2051 - (zSP - zStack), (zSP - zFP->sp) - 1, *(zSP)); 
    return *(zSP);
}

zword_t stack_poke(zword_t value) {
    LOG(ZDEBUG, "\nStack: push - size now %li, frame usage %li (value #%x)", 2051 - (zSP - zStack), (zSP - zFP->sp) - 1, value); 
    return *(zSP) = value;
}

zstack_frame_t * call_zroutine(packed_addr_t address, zoperand_t *operands, zbyte_t ret_store, int keep_return){
    zbyte_t local_count;
    zstack_frame_t *newFrame;
    int i = 0;
		packed_addr_t routine;

		routine = address;

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
	newFrame->ret_keep = keep_return;
   
	local_count = get_byte(address++);
	if (zGameVersion < Z_VERSION_5) {
		/* defaults for locals stored after local count in V3/V4 */
    for (i = 0; i < local_count; i++) {
        newFrame->locals[i] = get_word(address);
        address += 2;
    }
		while (i++ < 16)
			newFrame->locals[i] = 0;
	} else {
		/* in V5 and above locals have no defaults and are zeroed */
    for (i = 0; i < local_count; i++)
			newFrame->locals[i] = 0;
	}
   
	i = 0;
    while (operands->type != NONE) {
		newFrame->args |= (1 << i);
        newFrame->locals[i++] = operands->type == VARIABLE ? variable_get(operands->bytes) : operands->bytes;
        operands++;
    }
    
	LOG(ZDEBUG, "\nCALL $%x -> V%03i", routine, ret_store)
    zPC = address;
    zSP++;
    
    return zFP++;
}

zstack_frame_t *return_zroutine(zword_t ret_value) {
    zbyte_t ret_store;
	int ret_keep;
    
    if ((zFP - 1) < zCallStack)
        fatal_error("Call stack underflow");
    
    LOG(ZDEBUG, "\nReturned %i into V%x (%05x)", ret_value, zFP->ret_store, zFP->pc)
	LOG(ZDEBUG, "\nStack: returned, discarded %li outstanding items (stack top now #%hn, size %li, frame usage %li)",
				zSP - zFP->sp, (zFP - 1)->sp, (zSP - (zFP - 1)->sp) - (zSP - zFP->sp),(zFP - 1) - zCallStack)

    zSP = zFP->sp;
    zPC = zFP->pc;
    ret_store = zFP->ret_store;
	ret_keep = zFP->ret_keep;

    zFP--;
    
	if (ret_keep)
	    variable_set(ret_store, ret_value);

    return zFP;
}
