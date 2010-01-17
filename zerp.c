/* 
    Zerp: a Z-machine interpreter
    zerp.c : main z-machine interpreter
*/

#include <stdlib.h>
#include "glk.h"
#include "zerp.h"
#include "opcodes.h"

zByteAddr * zStack = 0;
zByteAddr * zSP = 0;
zByteAddr * zFP = 0;
zByteAddr zGlobals = 0;
unsigned short zPC = 0;

inline static int decode_variable(unsigned char optypes, zByteAddr *operands);
inline static int decode_short(unsigned char opbyte, zByteAddr *operands);
inline static int decode_long(unsigned char opbyte, zByteAddr *operands);

/* main interpreter entrypoint */
int zerp_run() {
    unsigned char op;
    int opsize, opcode, opcount, var_opcount;
    zByteAddr operands[8];
    
    /* intialise the stack and pc */
    zStack = calloc(STACKSIZE, sizeof(zByteAddr));
    if (!zStack) {
        glk_put_string("Failed to allocate stack space!\n");
        return;
    }
    zSP = zStack; zFP = zSP;
    zPC = byte_addr(PC_INITIAL);
    zGlobals = byte_addr(GLOBALS);
    int inx;
    for (inx = 0; inx < 8; inx++)
        operands[inx] = 0;
        
    glk_put_string("Running...\n");
    
    int max = 6;
    while (max--) {
        glk_printf("%#0x : ", zPC);

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
            glk_printf("Unsupported extended opcode %x @ %#0x\n", game_byte(zPC++));
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
    
        glk_printf("byte: %#x, opcode: %#0x, opcount: %#0x, opsize: %#0x\n", op, opcode, opcount, opsize);

        switch (opsize) {
            case OP_SHORT:
                var_opcount = decode_short(op, &operands);
                break;
            case OP_LONG:
                var_opcount = decode_long(op, &operands);
                break;
            case OP_VARIABLE:
                var_opcount = decode_variable(game_byte(zPC++), &operands);
                break;
        }
        
        // glk_printf("operands: %#8x %#8x %#8x %#8x %#8x %#8x %#8x %#8x\n",
        //             operands[0], operands[1], operands[2], operands[3],
        //             operands[4], operands[5], operands[6], operands[7]);

        switch (opcount) {
            case COUNT_2OP:
                switch (opcode) {
                    case JE:
                    break;
                    case JL:
                    break;
                    case JG:
                    break;
                    case DEC_CHK:
                    break;
                    case INC_CHK:
                    break;
                    case JIN:
                    break;
                    case TEST:
                    break;
                    case OR:
                    break;
                    case AND:
                    break;
                    case TEST_ATTR:
                    break;
                    case SET_ATTR:
                    break;
                    case CLEAR_ATTR:
                    break;
                    case STORE:
                    glk_printf("@store   %#x -> %#x\n", operands[1], operands[0]);
                    variable_set(operands[0], operands[1]);
                    break;
                    case INSERT_OBJ:
                    break;
                    case LOADW:
                    break;
                    case LOADB:
                    break;
                    case GET_PROP:
                    break;
                    case GET_PROP_ADDR:
                    break;
                    case GET_NEXT_PROP:
                    break;
                    case ADD:
                    glk_printf("@add %#x + %#x -> %#x\n", operands[0], operands[1], game_byte(zPC++));
                    break;
                    case SUB:
                    break;
                    case MUL:
                    break;
                    case DIV:
                    break;
                    case MOD:
                    break;
                }
                break;
            case COUNT_1OP:
                switch (opcode) {
                    case JZ:
                    break;
                    case GET_SIBLING:
                    break;
                    case GET_CHILD:
                    break;
                    case GET_PARENT:
                    break;
                    case GET_PROP_LEN:
                    break;
                    case INC:
                    break;
                    case DEC:
                    break;
                    case PRINT_ADDR:
                    break;
                    case REMOVE_OBJ:
                    break;
                    case PRINT_OBJ:
                    break;
                    case RET:
                    break;
                    case JUMP:
                    break;
                    case PRINT_PADDR:
                    break;
                    case LOAD:
                    break;
                    case NOT:
                    glk_printf("@not %#x -> %#x\n", operands[0], game_byte(zPC++));
                    break;
                }
                break;
            case COUNT_0OP:
                switch (opcode) {
                    case RTRUE:
                    break;
                    case RFALSE:
                    break;
                    case PRINT:
                    glk_put_string("@print \"");
                    zPC += print_zstring(zPC);
                    glk_put_string("\"\n");
                    break;
                    case PRINT_RET:
                    break;
                    case NOP:
                    break;
                    case SAVE:
                    break;
                    case RESTORE:
                    break;
                    case RESTART:
                    break;
                    case RET_POPPED:
                    glk_put_string("@ret_popped\n");
                    break;
                    case POP:
                    break;
                    case QUIT:
                    break;
                    case NEW_LINE:
                    glk_printf("@new_line\n");
                    glk_put_string("\n");
                    break;
                    case SHOW_STATUS:
                    glk_printf("@show_status\n");
                    break;
                    case VERIFY:
                    break;
                }
                break;
            case COUNT_VAR:
                switch (opcode) {
                    int i;
                    case CALL:
                    glk_printf("@call %#x (", unpack(operands[0]));
                    for (i = 1; i < var_opcount; i++) {
                        glk_printf("%#x ", operands[i]);
                    }
                    glk_printf(") -> %#x\n", game_byte(zPC++));
                    break;
                    case STOREW:
                    break;
                    case STOREB:
                    break;
                    case PUT_PROP:
                    break;
                    case SREAD:
                    break;
                    case PRINT_CHAR:
                    break;
                    case PRINT_NUM:
                    break;
                    case PUSH:
                    break;
                    case PULL:
                    break;
                    case SPLIT_WINDOW:
                    break;
                    case SET_WINDOW:
                    break;
                    case OUTPUT_STREAM:
                    break;
                    case INPUT_STREAM:
                    break;
                    case SOUND_EFFECT:
                    break;
                }
                break;
            default:
                glk_put_string("Unknown opcode error");
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
                break;
            case SMALL_CONST:
                *operands++ = (zByteAddr) game_byte(zPC++);
                break;
            case VARIABLE:
                *operands++ = variable_get(game_byte(zPC++));
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
            break;
        case SMALL_CONST:
            *operands = (zByteAddr) game_byte(zPC++);
            break;
        case VARIABLE:
            *operands = variable_get(game_byte(zPC++));
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
        } else {
            *operands++ = (zByteAddr) game_byte(zPC++);
        }
    }
    
    return 2;
}