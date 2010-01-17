/*
    Zerp: a Z-machine interpreter
    stack.c : z-machine stacks - value and call
*/

#include "glk.h"
#include "zerp.h"
#include "stack.h"

#if DEBUG > ZDEBUG
static void dump_stack() {
    zByteAddr *s;

    glk_put_string("stack : ");
    
    s = zSP;
    while(--s >= zStack)
        glk_printf("%8x", *s);
        
    glk_put_string("\n");
}
#endif

int stack_push(zByteAddr value) {
#if DEBUG > ZDEBUG
    dump_stack();
#endif
    *(zSP++) = value;
    if (zSP >= zStackTop) {
        glk_put_string("ABORT: Value stack overflow!\n");
        glk_exit();
    }
}

zByteAddr stack_pop() {
#if DEBUG > ZDEBUG
    dump_stack();
#endif
    if (zSP == zStack) {
        return 0;
    }
    return *(--zSP);
}
