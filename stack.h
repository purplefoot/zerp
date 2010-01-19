/*
    Zerp: a Z-machine interpreter
    stack.h : z-machine stacks - value and call
*/

#ifndef STACK_H
#define STACK_H

int stack_push(zword_t value);
zword_t stack_pop();

zstack_frame_t *call_zroutine(packed_addr_t address, zword_t *operands, int opcount, zbyte_t ret_value);
zstack_frame_t *return_zroutine(zword_t ret_value);

#endif STACK_H