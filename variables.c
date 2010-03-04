/* 
    Zerp: a Z-machine interpreter
    variables.c : z-machine variable access
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


zword_t variable_get(zbyte_t variable) {
	zword_t value;

    if (variable == 0) {
        /* pop stack */
        value =  stack_pop();
    } else if (variable > 0 && variable < 0x10) {
        /* read a local */
        value = zFP->locals[variable - 1];
    } else {
        /* read a global */
        value =  get_word(zGlobals + ((variable - 0x10) * 2));
    }
    LOG(ZDEBUG, "\nRead variable #%x (value %i)", variable, value);
	return value;
}

zword_t indirect_variable_get(zbyte_t variable) {
	zword_t value;

    if (variable == 0) {
        /* just peek at the stack */
        value = stack_peek();
    } else if (variable > 0 && variable < 0x10) {
        /* read a local */
        value = zFP->locals[variable - 1];
    } else {
        /* read a global */
        value = get_word(zGlobals + ((variable - 0x10) * 2));
    }
    LOG(ZDEBUG, "\nRead variable #%x (value %i)", variable, value);
	return value;
}

int variable_set(zbyte_t variable, zword_t value) {
    LOG(ZDEBUG, "\nStoring %i in Variable #%x", value, variable);
    if (variable == 0) {
        /* push stack */
        return stack_push(value);
    } else if (variable > 0 && variable < 0x10) {
        /* write a local */
        return zFP->locals[variable - 1] = value;
    } else {
        /* write a global */
        store_word(zGlobals + ((variable - 0x10) *2), value);
        return value;
    }
}

int indirect_variable_set(zbyte_t variable, zword_t value) {
    LOG(ZDEBUG, "\nStoring %i in Variable #%x", value, variable);
    if (variable == 0) {
        /* poke stack */
        return stack_poke(value);
    } else if (variable > 0 && variable < 0x10) {
        /* write a local */
        return zFP->locals[variable - 1] = value;
    } else {
        /* write a global */
        store_word(zGlobals + ((variable - 0x10) *2), value);
        // LOG(ZDEBUG, "\nWrite G%02x (%04x)", variable - 0x10, value);
        return value;
    }
}