/* 
    Zerp: a Z-machine interpreter
    zerp.c : main z-machine interpreter
*/


#include <stdio.h>
#include <stdlib.h>
#include "glk.h"
#include "zerp.h"
#include "opcodes.h"

zByteAddr * zStack = 0;
zByteAddr * zSP = 0;
zByteAddr * zStackTop = 0;
zByteAddr * zFP = 0;
zByteAddr zGlobals = 0;
unsigned short zPC = 0;

#ifdef DEBUG
static char opdesc[10][16];
static int si, opind;
char *var_name(char *opstr, unsigned char byte);
#endif

inline static int decode_variable(unsigned char optypes, zByteAddr *operands);
inline static int decode_short(unsigned char opbyte, zByteAddr *operands);
inline static int decode_long(unsigned char opbyte, zByteAddr *operands);

/* main interpreter entrypoint */
int zerp_run() {
    unsigned char op, store_loc, branch, branch_long;
    int opsize, opcode, opcount, var_opcount;
    zByteAddr operands[8];


    /* intialise the stack and pc */
    zStack = calloc(STACKSIZE, sizeof(zByteAddr));
    if (!zStack) {
        glk_put_string("Failed to allocate stack space!\n");
        return;
    }
    zStackTop = zStack + STACKSIZE;
    
    zSP = zStack; zFP = zSP;
    zPC = byte_addr(PC_INITIAL);
    zGlobals = byte_addr(GLOBALS);
    int inx;
    for (inx = 0; inx < 8; inx++)
        operands[inx] = 0;
        
    LOG(ZDEBUG,"Running...\n", 0);
    
    int max = 26;
    while (max--) {
        LOG(ZDEBUG,"%#0x : ", zPC);
#ifdef DEBUG
        for (si = 0; si < 9; si++)
            opdesc[si][0] = 0;
        opind = 0;
#endif

        op = game_byte(zPC++);
    
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
            LOG(ZERROR, "Unsupported extended opcode %x @ %#0x\n", game_byte(zPC++));
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
    
        LOG(ZCRAZY, "byte: %#x, opcode: %#0x, opcount: %#0x, opsize: %#0x\n", op, opcode, opcount, opsize);

        switch (opsize) {
            case OP_SHORT:
                var_opcount = decode_short(op, (zByteAddr *)&operands);
                break;
            case OP_LONG:
                var_opcount = decode_long(op, (zByteAddr *)&operands);
                break;
            case OP_VARIABLE:
                var_opcount = decode_variable(game_byte(zPC++), (zByteAddr *)&operands);
                break;
        }
        
        LOG(ZCRAZY, "operands: %8s %8s %8s %8s %8s %8s %8s %8s\n",
                    opdesc[0], opdesc[1], opdesc[2], opdesc[3],
                    opdesc[4], opdesc[5], opdesc[6], opdesc[7]);
        LOG(ZCRAZY, "operands: %#8x %#8x %#8x %#8x %#8x %#8x %#8x %#8x\n",
                    operands[0], operands[1], operands[2], operands[3],
                    operands[4], operands[5], operands[6], operands[7]);

        switch (opcount) {
            case COUNT_2OP:
                switch (opcode) {
                    case JE:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@je %#s : %#s  %#s %#s, %#s\n", opdesc[0], opdesc[1], opdesc[2], opdesc[3], var_name((char *)&opdesc[9], branch));
                    break;
                    case JL:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@jl %#s < %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case JG:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@jg %#s > %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case DEC_CHK:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@dec_check %#s, %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case INC_CHK:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@inc_check %#s, %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case JIN:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@jin %#s in %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case TEST:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@test %#s, %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case OR:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@or %#s | %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case AND:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@and %#s & %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case TEST_ATTR:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@test_attr %#s, %#s, %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[9], branch));
                    break;
                    case SET_ATTR:
                    LOG(ZDEBUG, "@set_attr %#s, %#s\n", opdesc[0], opdesc[1]);
                    break;
                    case CLEAR_ATTR:
                    LOG(ZDEBUG, "@clear_attr %#s, %#s\n", opdesc[0], opdesc[1]);
                    break;
                    case STORE:
                    LOG(ZDEBUG, "@store   %#s, %#s\n", var_name((char *)&opdesc[0], operands[0]), opdesc[1]);
                    variable_set(operands[0], operands[1]);
                    break;
                    case INSERT_OBJ:
                    LOG(ZDEBUG, "@insert_obj %#s, %#s\n", opdesc[0], opdesc[1]);
                    break;
                    case LOADW:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@loadw %#s->%#s -> %#s \n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case LOADB:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@loadb %#s->%#s -> %#s \n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case GET_PROP:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_prop %#s,%#s -> %#s \n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case GET_PROP_ADDR:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_prop_addr %#s,%#s -> %#s \n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case GET_NEXT_PROP:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_next_prop %#s,%#s -> %#s \n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case ADD:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@add %#s + %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case SUB:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@sub %#s - %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case MUL:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@mul %#s * %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case DIV:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@div %#s / %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case MOD:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@mod %#s %% %#s -> %#s\n", opdesc[0], opdesc[1], var_name((char *)&opdesc[8], store_loc));
                    break;
                }
                break;
            case COUNT_1OP:
                switch (opcode) {
                    case JZ:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@jz %#s, %#s\n", opdesc[0], var_name((char *)&opdesc[9], branch));
                    break;
                    case GET_SIBLING:
                    store_loc = game_byte(zPC++);
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_sibling %#s -> %#s, %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc),
                                                                             var_name((char *)&opdesc[9], branch));
                    break;
                    case GET_CHILD:
                    store_loc = game_byte(zPC++);
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_child %#s -> %#s, %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc),
                                                                             var_name((char *)&opdesc[9], branch));

                    break;
                    case GET_PARENT:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_parent %#s -> %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case GET_PROP_LEN:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@get_prop_len %#s -> %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case INC:
                    LOG(ZDEBUG, "@inc %#s \n", opdesc[0]);
                    break;
                    case DEC:
                    LOG(ZDEBUG, "@dec %#s \n", opdesc[0]);
                    break;
                    case PRINT_ADDR:
                    LOG(ZDEBUG, "@print_addr %#s \n", opdesc[0]);
                    break;
                    case REMOVE_OBJ:
                    LOG(ZDEBUG, "@remove_obj %#s \n", opdesc[0]);
                    break;
                    case PRINT_OBJ:
                    LOG(ZDEBUG, "@print_obj %#s \n", opdesc[0]);
                    break;
                    case RET:
                    LOG(ZDEBUG, "@ret %#s \n", opdesc[0]);
                    break;
                    case JUMP:
                    LOG(ZDEBUG, "@jump %#s \n", opdesc[0]);
                    break;
                    case PRINT_PADDR:
                    LOG(ZDEBUG, "@print_paddr %#s \n", opdesc[0]);
                    break;
                    case LOAD:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@load %#s -> %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc));
                    break;
                    case NOT:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@not !%#s -> %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc));
                    break;
                }
                break;
            case COUNT_0OP:
                switch (opcode) {
                    case RTRUE:
                    LOG(ZDEBUG, "@rtrue\n", 0);
                    break;
                    case RFALSE:
                    LOG(ZDEBUG, "@rfalse\n", 0);
                    break;
                    case PRINT:
                    LOG(ZDEBUG, "@print \"", 0);
                    zPC += print_zstring(zPC);
                    LOG(ZDEBUG, "\"\n", 0);
                    break;
                    case PRINT_RET:
                    LOG(ZDEBUG, "@print_ret \"", 0);
                    zPC += print_zstring(zPC);
                    LOG(ZDEBUG, "\"\n", 0);
                    glk_put_string("\n");
                    break;
                    case NOP:
                    LOG(ZDEBUG, "@nop\n", 0);
                    break;
                    case SAVE:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@save, %#s\n", var_name((char *)&opdesc[9], branch));
                    break;
                    case RESTORE:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@restore, %#s\n", var_name((char *)&opdesc[9], branch));
                    break;
                    case RESTART:
                    LOG(ZDEBUG, "@restart\n", 0);
                    break;
                    case RET_POPPED:
                    LOG(ZDEBUG, "@ret_popped\n", 0);
                    break;
                    case POP:
                    LOG(ZDEBUG, "@pop\n", 0);
                    break;
                    case QUIT:
                    LOG(ZDEBUG, "@quit\n", 0);
                    break;
                    case NEW_LINE:
                    LOG(ZDEBUG, "@new_line\n", 0);
                    glk_put_string("\n");
                    break;
                    case SHOW_STATUS:
                    LOG(ZDEBUG, "@show_status\n", 0);
                    break;
                    case VERIFY:
                    branch = game_byte(zPC++);
                    if (!(branch >> 6 & 1))
                        branch_long = game_byte(zPC++);
                    LOG(ZDEBUG, "@verify, %#s\n", var_name((char *)&opdesc[9], branch));
                    break;
                }
                break;
            case COUNT_VAR:
                switch (opcode) {
                    int i;
                    case CALL:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@call %#x (", unpack(operands[0]));
                    for (i = 1; i < var_opcount; i++) {
                        LOG(ZDEBUG, "%#s ", opdesc[i]);
                    }
                    LOG(ZDEBUG, ") -> %#s\n", var_name((char *)&opdesc[8], store_loc));
                    /* phony return value */
                    stack_push(0);
                    break;
                    case STOREW:
                    LOG(ZDEBUG, "@storew %#s->%#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
                    break;
                    case STOREB:
                    LOG(ZDEBUG, "@storeb %#s->%#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
                    break;
                    case PUT_PROP:
                    LOG(ZDEBUG, "@put_prop %#s %#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
                    break;
                    case SREAD:
                    LOG(ZDEBUG, "@sread %#s %#s\n", opdesc[0], opdesc[1]);
                    break;
                    case PRINT_CHAR:
                    LOG(ZDEBUG, "@print_char %#s\n", opdesc[0]);
                    break;
                    case PRINT_NUM:
                    LOG(ZDEBUG, "@print_num %#s\n", opdesc[0]);
                    break;
                    case RANDOM:
                    store_loc = game_byte(zPC++);
                    LOG(ZDEBUG, "@random %#s -> %#s\n", opdesc[0], var_name((char *)&opdesc[8], store_loc));
                    case PUSH:
                    LOG(ZDEBUG, "@push %#s -> SP\n", opdesc[0]);
                    break;
                    case PULL:
                    LOG(ZDEBUG, "@pull SP -> %#s\n", opdesc[0]);
                    break;
                    case SPLIT_WINDOW:
                    LOG(ZDEBUG, "@split_window %#s\n", opdesc[0]);
                    break;
                    case SET_WINDOW:
                    LOG(ZDEBUG, "@set_window %#s\n", opdesc[0]);
                    break;
                    case OUTPUT_STREAM:
                    LOG(ZDEBUG, "@output_stream %#s\n", opdesc[0]);
                    break;
                    case INPUT_STREAM:
                    LOG(ZDEBUG, "@input_stream %#s\n", opdesc[0]);
                    break;
                    case SOUND_EFFECT:
                    LOG(ZDEBUG, "@sound_effect %#s\n", opdesc[0]);
                    break;
                }
                break;
            default:
                LOG(ZERROR, "Unknown opcode: %#x", op);
                glk_put_string("ABORT: Unknown Z-machine opcode.");
        }
    }
}

/*
    decode_variable - decode operands for variable form opcodes

    4.4.3
    In variable or extended forms, a byte of 4 operand types is given next. This contains 4 2-bit
    fields: bits 6 and 7 are the first field, bits 0 and 1 the fourth. The values are operand types
    as above. Once one type has been given as 'omitted', all subsequent ones must be. Example: 
    $$00101111 means large constant followed by variable (and no third or fourth opcode).
*/
inline static int decode_variable(unsigned char optypes, zByteAddr *operands) {
    int shift = 6, opcount = 0, optype;
    
    while (shift >= 0 && (optype = (optypes >> shift) & 0x3) != 0x3) {
        switch (optype) {
            case LARGE_CONST:
                *operands++ = byte_addr(zPC++);
                zPC++;
#ifdef DEBUG
                snprintf((char *)&opdesc[opind++], 16, "%#8x", byte_addr(zPC - 2));
#endif
                break;
            case SMALL_CONST:
                *operands++ = (zByteAddr) game_byte(zPC++);
#ifdef DEBUG
                snprintf((char *)&opdesc[opind++], 16, "%#8x", game_byte(zPC - 1));
#endif
                break;
            case VARIABLE:
                *operands++ = variable_get(game_byte(zPC++));
#ifdef DEBUG
                var_name((char *)&opdesc[opind++], game_byte(zPC - 1));
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
inline static int decode_short(unsigned char opbyte, zByteAddr *operands) {
    switch ((opbyte >> 4) & 0x3) {
        case LARGE_CONST:
            *operands = byte_addr(zPC++);
            zPC++;
#ifdef DEBUG
            snprintf((char *)&opdesc[opind++], 16, "%#8x", byte_addr(zPC - 2));
#endif
            break;
        case SMALL_CONST:
            *operands = (zByteAddr) game_byte(zPC++);
#ifdef DEBUG
            snprintf((char *)&opdesc[opind++], 16, "%#8x", game_byte(zPC - 1));
#endif
            break;
        case VARIABLE:
            *operands = variable_get(game_byte(zPC++));
#ifdef DEBUG
            var_name((char *)&opdesc[opind++], game_byte(zPC - 1));
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
inline static int decode_long(unsigned char opbyte, zByteAddr *operands) {
    int bit;
    
    for (bit = 6; bit > 4; bit--) {
        if ((opbyte >> bit) & 0x1) {
            *operands++ = variable_get(game_byte(zPC++));
#ifdef DEBUG
            var_name((char *)&opdesc[opind++], game_byte(zPC - 1));
#endif
        } else {
            *operands++ = (zByteAddr) game_byte(zPC++);
#ifdef DEBUG
            snprintf((char *)&opdesc[opind++], 16, "%#8x", game_byte(zPC - 1));
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
        snprintf(opstr, 16, "   L%#2x", byte);
    } else {
        snprintf(opstr, 16, "   G%#2x", byte - 0x10);
    }
    return opstr;
}
#endif