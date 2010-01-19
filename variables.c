/* 
    Zerp: a Z-machine interpreter
    variables.c : z-machine variable access
*/

#include "glk.h"
#include "zerp.h"
#include "stack.h"


zword_t variable_get(zbyte_t variable) {
    if (variable == 0) {
        /* pop stack */
        return stack_pop();
    } else if (variable > 0 && variable < 0x10) {
        /* read a local */
        // LOG(ZDEBUG, "Read L%02x (%04x)\n", variable - 1, zFP->locals[variable - 1]);
        return zFP->locals[variable - 1];
    } else {
        /* read a global */
        // LOG(ZDEBUG, "Read G%02x (%04x)\n", variable - 0x10, get_word(zGlobals + ((variable - 0x10) * 2)));
        return get_word(zGlobals + ((variable - 0x10) * 2));
    }
}

int variable_set(zbyte_t variable, zword_t value) {
    if (variable == 0) {
        /* push stack */
        return stack_push(value);
    } else if (variable > 0 && variable < 0x10) {
        /* write a local */
        // LOG(ZDEBUG, "Write L%02x (%04x)\n", variable - 1, value);
        return zFP->locals[variable - 1] = value;
    } else {
        /* write a global */
        store_word(zGlobals + ((variable - 0x10) *2), value);
        // LOG(ZDEBUG, "Write G%02x (%04x)\n", variable - 0x10, value);
        return value;
    }
}