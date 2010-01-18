/*
    Zerp: a Z-machine interpreter
    stack.h : z-machine stacks - value and call
*/

#ifndef STACK_H
#define STACK_H

int stack_push(zword_t value);
zword_t stack_pop();

#endif STACK_H