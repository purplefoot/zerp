/*
    Zerp: a Z-machine interpreter
    stack.h : z-machine stacks - value and call
*/

#ifndef STACK_H
#define STACK_H

int stack_push(zword_t value);
zword_t stack_pop();

zstack_frame_t *call_zroutine(packed_addr_t address, zoperand_t *operands, zbyte_t ret_store);
zstack_frame_t *return_zroutine(zword_t ret_store);

#endif STACK_H