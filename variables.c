/* 
    Zerp: a Z-machine interpreter
    variables.c : z-machine variable access
*/

#include "glk.h"
#include "zerp.h"
#include "stack.h"

static void dump_globals() {
    int g, x, y;
    
    glk_put_string("        ");
    for (x = 0; x < 16; x++)
        glk_printf("%8x", x);
    glk_put_string("\n");

    for (g = 0; g < 240; g++) {
        if (!(g % 16))
            glk_printf("\n%8x", g);
        glk_printf("%8x", byte_addr(zGlobals + ((g) * 2)));
    }
    glk_put_string("\n");
}

zByteAddr variable_get(unsigned char variable) {
    if (variable == 0) {
        /* pop stack */
        return stack_pop();
    } else if (variable > 0 && variable < 0x10) {
        /* read a local */
        glk_printf("Reading local %#x\n", variable);
        return 0;
    } else {
        /* read a global */
        glk_printf("Reading global %#x\n", variable - 0x10);
        // dump_globals();
        return byte_addr(zGlobals + ((variable - 0x10) * 2));
    }
}

int variable_set(unsigned char variable, zByteAddr value) {
    if (variable == 0) {
        /* push stack */
        return stack_push(value);
    } else if (variable > 0 && variable < 0x10) {
        /* write a local */
        glk_printf("Writing local %#x\n", variable);
        return 0;
    } else {
        /* write a global */
        glk_printf("Writing global %#x\n", variable - 0x10);
        store_byte_addr(zGlobals + ((variable - 0x10) *2), value);
        // dump_globals();
        return 1;
    }
}