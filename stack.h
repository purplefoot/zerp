/*
    Zerp: a Z-machine interpreter
    stack.h : z-machine stacks - value and call
*/

#ifndef STACK_H
#define STACK_H

int stack_push(zByteAddr value);
zByteAddr stack_pop();

#endif STACK_H