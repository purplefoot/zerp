/*
    Zerp: a Z-machine interpreter
    stack.c : z-machine stacks - value and call
*/

#include "glk.h"
#include "zerp.h"
#include "stack.h"

void dump_stack() {
    zByteAddr *s;

    glk_put_string("stack : ");
    
    s = zSP;
    while(--s >= zStack)
        glk_printf("%8x", *s);
        
    glk_put_string("\n");
}

int stack_push(zByteAddr value) {
    dump_stack();
    *(zSP++) = value;
    if (zSP >= zStackTop) {
        glk_put_string("ABORT: Value stack overflow!\n");
        glk_exit();
    }
}

zByteAddr stack_pop() {
    dump_stack();
    if (zSP == zStack) {
        return 0;
    }
    return *(--zSP);
}
