/* 
    Zerp: a Z-machine interpreter
    zerp.c : main z-machine interpreter
*/

#include <stdio.h>
#include <stdlib.h>
#include "glk.h"
#include "zerp.h"
#include "opcodes.h"
#include "stack.h"
#include "debug.h"

zword_t * zStack = 0;
zword_t * zSP = 0;
zword_t * zStackTop = 0;
zstack_frame_t * zCallStack = 0;
zstack_frame_t * zFP = 0;
zstack_frame_t * zCallStackTop = 0;
zword_t zGlobals = 0;
packed_addr_t zPC = 0;

#ifdef DEBUG
static char opdesc[10][16];
static int si, opind;
char *var_name(char *opstr, unsigned char byte);
#endif

inline static int decode_variable(unsigned char optypes, zword_t *operands);
inline static int decode_short(unsigned char opbyte, zword_t *operands);
inline static int decode_long(unsigned char opbyte, zword_t *operands);

/* main interpreter entrypoint */
int zerp_run() {
    zbyte_t op, store_loc, branch, branch_long;
    int opsize, opcode, opcount, var_opcount, running;
    zword_t operands[8];
    signed short boffset;


    /* intialise the stack and pc */
    zStack = calloc(STACKSIZE, sizeof(zword_t));
    zStackTop = zStack + STACKSIZE;
    zCallStack = calloc(CALLSTACKSIZE, sizeof(zstack_frame_t));
    zCallStackTop = zCallStack + CALLSTACKSIZE;
    if (!zStack || !zCallStack) {
        glk_put_string("Failed to allocate stack space!\n");
        return;
    }
    
    zSP = zStack;
    zFP = zCallStack;
    zFP->sp = zSP;
    zPC = get_word(PC_INITIAL);
    zGlobals = get_word(GLOBALS);
    int inx;
    for (inx = 0; inx < 8; inx++)
        operands[inx] = 0;
        
    LOG(ZDEBUG,"Running...\n", 0);
    
    running = TRUE;
    
    while (running) {
        // debug_monitor();
        
        LOG(ZDEBUG,"\n%#07x : ", zPC);
#ifdef DEBUG
        for (si = 0; si < 9; si++)
            opdesc[si][0] = 0;
        opind = 0;
#endif

        op = get_byte(zPC++);
    
        /*
            decode opcode
            
            4.3
            Each instruction has a form (long, short, extended or variable) and an operand count
            (0OP, 1OP, 2OP or VAR). If the top two bits of the opcode are $$11 the form is variable;
            if $$10, the form is short. If the opcode is 190 ($BE in hexadecimal) and the version
            is 5 or later, the form is "extended". Otherwise, the form is "long".
        */
        if (op == OP_EXTENDED) {
            /*
                4.3.4
                In extended form, the operand count is VAR. The opcode number is given in a second
                opcode byte.
            */
            opsize = OP_VARIABLE;
            LOG(ZERROR, "Unsupported extended opcode %#02x @ %#04x\n", get_byte(zPC++));
        } else if (op >> 6 == OP_VARIABLE ) {
            /*
                4.3.3
                In variable form, if bit 5 is 0 then the count is 2OP; if it is 1, then the count is
                VAR. The opcode number is given in the bottom 5 bits.  
            */
            opsize = OP_VARIABLE;
            opcount = (op >> 5) & 0x01 ? COUNT_VAR : COUNT_2OP;
            opcode = op & OPCODE_5BIT;        
        } else if (op >> 6 == OP_SHORT) {
            /*
                4.3.1
                In short form, bits 4 and 5 of the opcode byte give an operand type as above. If
                this is $11 then the operand count is 0OP; otherwise, 1OP. In either case the
                opcode number is given in the bottom 4 bits.
            */
            opsize = OP_SHORT;
            opcount = (op >> 4) & 0x03 == 0x03 ? COUNT_0OP : COUNT_1OP;
            opcode = op & OPCODE_4BIT;
        } else { /* OP_LONG */
            /*
                4.3.2
                In long form the operand count is always 2OP. The opcode number is given in the
                bottom 5 bits.
            */
            opsize = OP_LONG;
            opcount = COUNT_2OP;
            opcode = op & OPCODE_5BIT;
        }
    
        LOG(ZCRAZY, "byte: %#02x, opcode: %#02x, opcount: %#02x, opsize: %#02x\n", op, opcode, opcount, opsize);

        switch (opsize) {
            case OP_SHORT:
                var_opcount = decode_short(op, (zword_t *)&operands);
                break;
            case OP_LONG:
                var_opcount = decode_long(op, (zword_t *)&operands);
                break;
            case OP_VARIABLE:
                var_opcount = decode_variable(get_byte(zPC++), (zword_t *)&operands);
                break;
        }
        
        LOG(ZCRAZY, "operands: %#04x %#04x %#04x %#04x %#04x %#04x %#04x %#04x\n",
                    operands[0], operands[1], operands[2], operands[3],
                    operands[4], operands[5], operands[6], operands[7]);
        LOG(ZCRAZY, "opvalues: %04s %04s %04s %04s %04s %04s %04s %04s\n",
                    opdesc[0], opdesc[1], opdesc[2], opdesc[3],
                    opdesc[4], opdesc[5], opdesc[6], opdesc[7]);
                    
        switch (opcount) {
            case COUNT_2OP:
                switch (opcode) {
                    case JE:
                        branch_op("#JE %#s : %#s %#s %#s", operands[0] == operands[1])
                        break;
                    case JL:
                        branch_op("JL %#s < %#s", operands[0] < operands[1])
                        break;
                    case JG:
                        branch_op("JL %#s > %#s", operands[0] > operands[1])
                        break;
                    case DEC_CHK:
                        branch_op("DEC_CHECK %#s, %#s", variable_set(operands[0], variable_get(operands[0]) - 1) < operands[1])
                        break;
                    case INC_CHK:
                        branch_op("INC_CHECK %#s, %#s", variable_set(operands[0], variable_get(operands[0]) + 1) > operands[1])
                        break;
                    case JIN:
                        branch_op("#JIN %#s in %#s", 0)
                        break;
                    case TEST:
                        branch_op("TEST %#s &= %#s", operands[0] & operands[1] == operands[1])
                        break;
                    case OR:
                        store_op("OR %#s | %#s", operands[0] | operands[1])
                        break;
                    case AND:
                        store_op("AND %#s | %#s", operands[0] & operands[1])
                        break;
                    case TEST_ATTR:
                        branch_op("#TEST_ATTR %#s, %#s, ", 0)
                        break;
                    case SET_ATTR:
                        LOG(ZDEBUG, "#SET_ATTR %#s, %#s\n", opdesc[0], opdesc[1]);
                        break;
                    case CLEAR_ATTR:
                        LOG(ZDEBUG, "#CLEAR_ATTR %#s, %#s\n", opdesc[0], opdesc[1]);
                        break;
                    case STORE:
                        LOG(ZDEBUG, "STORE %#s, %#s\n", var_name((char *)&opdesc[0], operands[0]), opdesc[1]);
                        variable_set(operands[0], operands[1]);
                        break;
                    case INSERT_OBJ:
                        LOG(ZDEBUG, "#INSERT_OBJ %#s, %#s\n", opdesc[0], opdesc[1]);
                        break;
                    case LOADW:
                        store_op("LOADW %#s[%#s]", get_word(operands[0] + operands[1] * 2))
                        break;
                    case LOADB:
                        store_op("LOADB %#s[%#s]", get_byte(operands[0] + operands[1]))
                        break;
                    case GET_PROP:
                        store_op("#GET_PROP %#s, %#s", 0xbeef)
                        break;
                    case GET_PROP_ADDR:
                        store_op("#GET_PROP_ADDR %#s, %#s", 0xbeef)
                        break;
                    case GET_NEXT_PROP:
                        store_op("#GET_NEXT_PROP %#s, %#s", 0xbeef)
                        break;
                    case ADD:
                        store_op("ADD %#s + %#s", (signed short)operands[0] + (signed short)operands[1])
                        break;
                    case SUB:
                        store_op("SUB %#s - %#s", (signed short)operands[0] - (signed short)operands[1])
                        break;
                    case MUL:
                        store_op("MUL %#s * %#s", (signed short)operands[0] * (signed short)operands[1])
                        break;
                    case DIV:
                        store_op("DIV %#s / %#s", (signed short)operands[0] / (signed short)operands[1])
                        break;
                    case MOD:
                        store_op("MOD %#s %% %#s", (signed short)operands[0] % (signed short)operands[1])
                        break;
                }
                break;
            case COUNT_1OP:
                switch (opcode) {
                    case JZ:
                        branch_op("JZ %#s", operands[0] == 0)
                        break;
                    case GET_SIBLING:
                        store_branch_op("#GET_SIBLING %#s", 0xbeef, 0)
                        break;
                    case GET_CHILD:
                        store_branch_op("#GET_CHILD %#s", 0xbeef, 0)
                        break;
                    case GET_PARENT:
                        store_op("#GET_PARENT %#s", 0xbeef)
                        break;
                    case GET_PROP_LEN:
                        store_op("#GET_PROP_LEN %#s", 0xbeef)
                        break;
                    case INC:
                        LOG(ZDEBUG, "INC %#s \n", opdesc[0]);
                        variable_set(operands[0], ((signed short)variable_get(operands[0])) + 1);
                        break;
                    case DEC:
                        LOG(ZDEBUG, "DEC %#s \n", opdesc[0]);
                        variable_set(operands[0], ((signed short)variable_get(operands[0])) - 1);
                        break;
                    case PRINT_ADDR:
                        LOG(ZDEBUG, "PRINT_ADDR %#s \"", opdesc[0]);
                        print_zstring(operands[0]);
                        LOG(ZDEBUG, "\"\n", 0);
                        break;
                    case REMOVE_OBJ:
                        LOG(ZDEBUG, "#REMOVE_OBJ %#s \n", opdesc[0]);
                        break;
                    case PRINT_OBJ:
                        LOG(ZDEBUG, "#PRINT_OBJ %#s \n", opdesc[0]);
                        break;
                    case RET:
                        LOG(ZDEBUG, "RET %#s \n", opdesc[0]);
                        return_zroutine(operands[0]);
                        break;
                    case JUMP:
                        LOG(ZDEBUG, "JUMP %#04x \n", (signed short) (zPC + operands[0] - 2));
                        zPC += (signed short) (operands[0] - 2);
                        break;
                    case PRINT_PADDR:
                        LOG(ZDEBUG, "PRINT_PADDR %#s \"", opdesc[0]);
                        print_zstring(unpack(operands[0]));
                        LOG(ZDEBUG, "\"\n", 0);
                        break;
                    case LOAD:
                        store_op("#LOAD %#s", 0)
                        break;
                    case NOT:
                        store_op("NOT !%#s", ~operands[0])
                        break;
                }
                break;
            case COUNT_0OP:
                switch (opcode) {
                    case RTRUE:
                        LOG(ZDEBUG, "RTRUE\n", 0);
                        return_zroutine(1);
                        break;
                    case RFALSE:
                        LOG(ZDEBUG, "RFALSE\n", 0);
                        return_zroutine(0);
                        break;
                    case PRINT:
                        LOG(ZDEBUG, "PRINT \"", 0);
                        zPC += print_zstring(zPC);
                        LOG(ZDEBUG, "\"\n", 0);
                        break;
                    case PRINT_RET:
                        LOG(ZDEBUG, "PRINT_RET \"", 0);
                        zPC += print_zstring(zPC);
                        LOG(ZDEBUG, "\"\n", 0);
                        glk_put_string("\n");
                        return_zroutine(1);
                        break;
                    case NOP:
                        LOG(ZDEBUG, "NOP\n", 0);
                        break;
                    case SAVE:
                        branch_op("#SAVE", 1)
                        break;
                    case RESTORE:
                        branch_op("#RESTORE", 1)
                        break;
                    case RESTART:
                        LOG(ZDEBUG, "#RESTART\n", 0);
                        break;
                    case RET_POPPED:
                        LOG(ZDEBUG, "RET_POPPED\n", 0);
                        return_zroutine(stack_pop());
                        break;
                    case POP:
                        LOG(ZDEBUG, "POP\n", 0);
                        stack_pop();
                        break;
                    case QUIT:
                        LOG(ZDEBUG, "QUIT\n", 0);
                        running = FALSE;
                        break;
                    case NEW_LINE:
                        LOG(ZDEBUG, "NEW_LINE\n", 0);
                        glk_put_string("\n");
                        break;
                    case SHOW_STATUS:
                        LOG(ZDEBUG, "#SHOW_STATUS\n", 0);
                        break;
                    case VERIFY:
                        branch_op("#VERIFY", 1)
                        break;
                }
                break;
            case COUNT_VAR:
                switch (opcode) {
                    case CALL:
                        store_loc = get_byte(zPC++);
                        LOG(ZDEBUG, "CALL %#04x (", unpack(operands[0]));
                        int i;
                        for (i = 1; i < var_opcount; i++) {
                            LOG(ZDEBUG, "%#s ", opdesc[i]);
                        }
                        LOG(ZDEBUG, ") -> %#s\n", var_name((char *)&opdesc[8], store_loc));
                        call_zroutine(unpack(operands[0]), &operands[1], var_opcount - 1, store_loc);
                        break;
                    case STOREW:
                        LOG(ZDEBUG, "STOREW %#s->%#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
                        store_word(operands[0] + operands[1] * 2, operands[2])
                        break;
                    case STOREB:
                        LOG(ZDEBUG, "STOREB %#s->%#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
                        store_byte(operands[0] + operands[1], operands[2])
                        break;
                    case PUT_PROP:
                        LOG(ZDEBUG, "#PUT_PROP %#s %#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
                        break;
                    case SREAD:
                        LOG(ZDEBUG, "#SREAD %#s %#s\n", opdesc[0], opdesc[1]);
                        break;
                    case PRINT_CHAR:
                        LOG(ZDEBUG, "PRINT_CHAR %#s\n", opdesc[0]);
                        glk_put_char(operands[0]);
                        break;
                    case PRINT_NUM:
                        LOG(ZDEBUG, "PRINT_NUM %#s\n", opdesc[0]);
                        glk_printf("%d", (signed short)operands[0]);
                        break;
                    case RANDOM:
                        store_loc = get_byte(zPC++);
                        LOG(ZDEBUG, "RANDOM %#s -> %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc));
                        if ((signed short)operands[0] < 0) {
                            srandom(operands[0]);
                            variable_set(store_loc, 0);
                        } else {
                            variable_set(store_loc, random());
                        }
                        break;
                    case PUSH:
                        LOG(ZDEBUG, "PUSH %#s -> SP\n", opdesc[0]);
                        stack_push(operands[0]);
                        break;
                    case PULL:
                        LOG(ZDEBUG, "#PULL SP -> %#s\n", opdesc[0]);
                        variable_set(operands[0], stack_pop);
                        break;
                    case SPLIT_WINDOW:
                        LOG(ZDEBUG, "#SPLIT_WINDOW %#s\n", opdesc[0]);
                        break;
                    case SET_WINDOW:
                        LOG(ZDEBUG, "#SET_WINDOW %#s\n", opdesc[0]);
                        break;
                    case OUTPUT_STREAM:
                        LOG(ZDEBUG, "#OUTPUT_STREAM %#s\n", opdesc[0]);
                        break;
                    case INPUT_STREAM:
                        LOG(ZDEBUG, "#INPUT_STREAM %#s\n", opdesc[0]);
                        break;
                    case SOUND_EFFECT:
                        LOG(ZDEBUG, "#SOUND_EFFECT %#s\n", opdesc[0]);
                        break;
                }
                break;
            default:
                LOG(ZERROR, "Unknown opcode: %#x", op);
                glk_put_string("ABORT: Unknown Z-machine opcode.");
        }
    }
    
    /* Done, so clean up */
    free(zStack);
    free(zCallStack);
}

/*
    decode_variable - decode operands for variable form opcodes

    4.4.3
    In variable or extended forms, a byte of 4 operand types is given next. This contains 4 2-bit
    fields: bits 6 and 7 are the first field, bits 0 and 1 the fourth. The values are operand types
    as above. Once one type has been given as 'omitted', all subsequent ones must be. Example: 
    $$00101111 means large constant followed by variable (and no third or fourth opcode).
*/
inline static int decode_variable(unsigned char optypes, zword_t *operands) {
    int shift = 6, opcount = 0, optype;
    
    while (shift >= 0 && (optype = (optypes >> shift) & 0x3) != 0x3) {
        switch (optype) {
            case LARGE_CONST:
                *operands++ = get_word(zPC++);
                zPC++;
#ifdef DEBUG
                snprintf((char *)&opdesc[opind++], 16, "%#04x", get_word(zPC - 2));
#endif
                break;
            case SMALL_CONST:
                *operands++ = (zword_t) get_byte(zPC++);
#ifdef DEBUG
                snprintf((char *)&opdesc[opind++], 16, "%#02x", get_byte(zPC - 1));
#endif
                break;
            case VARIABLE:
                *operands++ = variable_get(get_byte(zPC++));
#ifdef DEBUG
                var_name((char *)&opdesc[opind++], get_byte(zPC - 1));
#endif
                break;
        }
        shift = shift -2; opcount++;
    }
    return opcount;
}

/*
    decode_short - decode operands for short form opcodes

    4.4.1
    In short form, bits 4 and 5 of the opcode give the type.
*/
inline static int decode_short(unsigned char opbyte, zword_t *operands) {
    switch ((opbyte >> 4) & 0x3) {
        case LARGE_CONST:
            *operands = get_word(zPC++);
            zPC++;
#ifdef DEBUG
            snprintf((char *)&opdesc[opind++], 16, "%#04x", get_word(zPC - 2));
#endif
            break;
        case SMALL_CONST:
            *operands = (zword_t) get_byte(zPC++);
#ifdef DEBUG
            snprintf((char *)&opdesc[opind++], 16, "%#02x", get_byte(zPC - 1));
#endif
            break;
        case VARIABLE:
            *operands = variable_get(get_byte(zPC++));
#ifdef DEBUG
            var_name((char *)&opdesc[opind++], get_byte(zPC - 1));
#endif
            break;
    }
    
    return 1;
}

/*
    decode_long - decode operands for long form opcodes

    4.4.2
    In long form, bit 6 of the opcode gives the type of the first operand, bit 5 of the second. A 
    value of 0 means a small constant and 1 means a variable. (If a 2OP instruction needs a large
    constant as operand, then it should be assembled in variable rather than long form.)
*/
inline static int decode_long(unsigned char opbyte, zword_t *operands) {
    int bit;
    
    for (bit = 6; bit > 4; bit--) {
        if ((opbyte >> bit) & 0x1) {
            *operands++ = variable_get(get_byte(zPC++));
#ifdef DEBUG
            var_name((char *)&opdesc[opind++], get_byte(zPC - 1));
#endif
        } else {
            *operands++ = (zword_t) get_byte(zPC++);
#ifdef DEBUG
            snprintf((char *)&opdesc[opind++], 16, "%#02x", get_byte(zPC - 1));
#endif
        }
    }
    
    return 2;
}

#ifdef DEBUG
char *var_name(char *opstr, unsigned char byte) {
    if (byte == 0) {
        snprintf(opstr, 16, "SP");
    } else if (byte < 0x10) {
        snprintf(opstr, 16, "L%02x", byte - 1);
    } else {
        snprintf(opstr, 16, "G%02x", byte - 0x10);
    }
    return opstr;
}
#endif