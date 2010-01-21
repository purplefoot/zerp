/* 
    Zerp: a Z-machine interpreter
    opcodes.c : opcode decoding
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glk.h"
#include "zerp.h"
#include "opcodes.h"

int decode_instruction(packed_addr_t pc, zinstruction_t *instruction, zoperand_t *operands, zword_t *store, zbranch_t *branch) {
    packed_addr_t startpc;
    
    startpc = pc;
    instruction->bytes = get_byte(pc++);
    /*
        decode opcode
        
        4.3
        Each instruction has a form (long, short, extended or variable) and an operand count
        (0OP, 1OP, 2OP or VAR). If the top two bits of the opcode are $$11 the form is variable;
        if $$10, the form is short. If the opcode is 190 ($BE in hexadecimal) and the version
        is 5 or later, the form is "extended". Otherwise, the form is "long".
    */
    if (instruction->bytes == OP_EXTENDED) {
        /*
            4.3.4
            In extended form, the operand count is VAR. The opcode number is given in a second
            opcode byte.
        */
        instruction->opcode = instruction->bytes;
        instruction->form = OP_VARIABLE;
        LOG(ZERROR, "Unsupported extended opcode %#02x @ %#04x\n", get_byte(pc++));
        decode_variable(&pc, instruction, get_byte(pc++), operands);
    } else if (instruction->bytes >> 6 == OP_VARIABLE ) {
        /*
            4.3.3
            In variable form, if bit 5 is 0 then the count is 2OP; if it is 1, then the count is
            VAR. The opcode number is given in the bottom 5 bits.  
        */
        instruction->form = OP_VARIABLE;
        instruction->count = (instruction->bytes >> 5) & 0x01 ? COUNT_VAR : COUNT_2OP;
        instruction->opcode = instruction->bytes & OPCODE_5BIT;
        decode_variable(&pc, instruction, get_byte(pc++), operands);   
    } else if (instruction->bytes >> 6 == OP_SHORT) {
        /*
            4.3.1
            In short form, bits 4 and 5 of the opcode byte give an operand type as above. If
            this is $11 then the operand count is 0OP; otherwise, 1OP. In either case the
            opcode number is given in the bottom 4 bits.
        */
        instruction->form = OP_SHORT;
        instruction->count = ((instruction->bytes >> 4) & 0x03) == 0x03 ? COUNT_0OP : COUNT_1OP;
        instruction->opcode = instruction->bytes & OPCODE_4BIT;
        if (instruction->count == COUNT_1OP)
            decode_short(&pc, instruction, operands);
    } else { /* OP_LONG */
        /*
            4.3.2
            In long form the operand count is always 2OP. The opcode number is given in the
            bottom 5 bits.
        */
        instruction->form = OP_LONG;
        instruction->count = COUNT_2OP;
        instruction->opcode = instruction->bytes & OPCODE_5BIT;
        decode_long(&pc, instruction, operands);
    }

    decode_store_and_branch(&pc, instruction, store, branch);

    return pc - startpc;
}

/*
    decode_variable - decode operands for variable form opcodes

    4.4.3
    In variable or extended forms, a byte of 4 operand types is given next. This contains 4 2-bit
    fields: bits 6 and 7 are the first field, bits 0 and 1 the fourth. The values are operand types
    as above. Once one type has been given as 'omitted', all subsequent ones must be. Example: 
    $$00101111 means large constant followed by variable (and no third or fourth opcode).
*/
inline static int decode_variable(packed_addr_t *pc, zinstruction_t* instruction, zbyte_t optypes, zoperand_t *operands) {
    int shift = 6, opcount = 0, optype;
    
    instruction->bytes = instruction->bytes << 8 | optypes;
    while (shift >= 0 && (optype = (optypes >> shift) & 0x3) != 0x3) {
        operands->type = optype;
        if (optype == LARGE_CONST) {
            (operands++)->bytes = get_word((*pc)++);
            (*pc)++;
        } else {
            (operands++)->bytes = (zword_t) get_byte((*pc)++);            
        }
        shift = shift - 2; opcount++;
    }
    /* tag the end of the operand list with NONE type */
    if (opcount < 8)
        operands->type = NONE;

    return opcount;
}

/*
    decode_short - decode operands for short form opcodes

    4.4.1
    In short form, bits 4 and 5 of the opcode give the type.
*/
inline static int decode_short(packed_addr_t *pc, zinstruction_t *instruction, zoperand_t *operands) {
    int optype;
    
    optype = (instruction->bytes >> 4) & 0x3;
    operands->type = optype;
    switch (optype) {
        case LARGE_CONST:
            (operands++)->bytes = get_word((*pc)++);
            (*pc)++;
            break;
        case SMALL_CONST:
            (operands++)->bytes = get_byte((*pc)++);
            break;
        case VARIABLE:
            (operands++)->bytes = (zword_t) get_byte((*pc)++);
            break;
    }
    
    /* tag the end of the operand list with NONE type */
    operands->type = NONE;
    
    return 1;
}

/*
    decode_long - decode operands for long form opcodes

    4.4.2
    In long form, bit 6 of the opcode gives the type of the first operand, bit 5 of the second. A 
    value of 0 means a small constant and 1 means a variable. (If a 2OP instruction needs a large
    constant as operand, then it should be assembled in variable rather than long form.)
*/
inline static int decode_long(packed_addr_t *pc, zinstruction_t *instruction, zoperand_t *operands) {
    int bit;
    
    for (bit = 6; bit > 4; bit--) {
        if ((instruction->bytes >> bit) & 0x1) {
            operands->type = VARIABLE;
        } else {
            operands->type = SMALL_CONST;
        }
        (operands++)->bytes = (zword_t) get_byte((*pc)++);
    }
    /* tag the end of the operand list with NONE type */
    operands->type = NONE;
    
    return 2;
}

int decode_store_and_branch(packed_addr_t *pc, zinstruction_t *instruction, zword_t *store, zbranch_t *branch) {
    switch (instruction->count) {
        case COUNT_2OP:
            switch (instruction->opcode) {
                case JE:
                case JL:
                case JG:
                case DEC_CHK:
                case INC_CHK:
                case JIN:
                case TEST:
                case TEST_ATTR:
                    decode_branch_op(pc, instruction, branch);
                    break;
                case OR:
                case AND:
                case LOADW:
                case INSERT_OBJ:
                case LOADB:
                case GET_PROP:
                case GET_PROP_ADDR:
                case GET_NEXT_PROP:
                case ADD:
                case SUB:
                case MUL:
                case DIV:
                case MOD:
                    decode_store_op(pc, instruction, store);
                    break;
                default:
                    break;
            }
            break;
        case COUNT_1OP:
            switch (instruction->opcode) {
                case JZ:
                    decode_branch_op(pc, instruction, branch);
                    break;
                case GET_SIBLING:
                case GET_CHILD:
                        decode_store_op(pc, instruction, store);
                        decode_branch_op(pc, instruction, branch);
                    break;
                case GET_PARENT:
                case GET_PROP_LEN:
                case LOAD:
                case NOT:
                    decode_store_op(pc, instruction, store);
                    break;
                default:
                    break;
            }
            break;
        case COUNT_VAR:
            switch (instruction->opcode) {
                case CALL:
                case RANDOM:
                    decode_store_op(pc, instruction, store);
                    break;
                default:
                    break;
            }
            break;
    }
}

static void decode_branch_op(packed_addr_t *pc, zinstruction_t *instruction, zbranch_t *branch) {
    int branch_short, branch_long;
    
    instruction->branch_flag = TRUE;
    branch_short = get_byte((*pc)++);
    branch->test = branch_short >> 7 & 1;
    if (!(branch_short >> 6 & 1)) {
        branch->type = BRANCH_LONG;
        branch_long = get_byte((*pc)++);
        branch->offset = branch_short & 0x1f;
        branch->offset = branch->offset << 8 | branch_long;
        branch->offset &= 0x1fff;
        branch->offset |= (branch_short >> 6 & 1) << 15;
        branch->bytes = branch_short << 8 | branch_long;
    } else {
        branch->type = BRANCH_SHORT;
        branch->offset = branch_short & 0x3f;
        branch->bytes = branch_short << 8;
    }
}

static void decode_store_op(packed_addr_t *pc, zinstruction_t *instruction, zword_t *store) {
    instruction->store_flag = TRUE;
    *store = get_byte((*pc)++);
}

void print_zinstruction(packed_addr_t instructionPC, zinstruction_t *instruction, zoperand_t *operands,
                        zword_t *store_operand, zbranch_t *branch_operand, int flags) {
    zoperand_t *op_ptr;
    char buf[32];
    int bytes_printed;

    glk_printf("\n%5x: ", instructionPC);
    if (!(flags & NO_BYTES)) {
        bytes_printed = 0;

        if (instruction->form == OP_VARIABLE) {
            glk_printf(" %02x %02x", instruction->bytes >> 8, instruction->bytes & 0xff);
            bytes_printed += 2;
        } else {
            glk_printf(" %02x", instruction->bytes);
            bytes_printed++;
        }

        if (instruction->count != COUNT_0OP) {
            op_ptr = operands;
            while(op_ptr->type != NONE) {
                if (op_ptr->type == LARGE_CONST) {
                    glk_printf(" %02x %02x", op_ptr->bytes >> 8, op_ptr->bytes & 0xff);
                    bytes_printed += 2;
                } else {
                    glk_printf(" %02x", op_ptr->bytes);
                    bytes_printed++;
                }
                op_ptr++;
            }            
        }

        if (instruction->store_flag){
            glk_printf(" %02x", *store_operand);
            bytes_printed++;
        }
        if (instruction->branch_flag) {
            if (branch_operand->type == BRANCH_LONG) {
                glk_printf(" %02x %02x", branch_operand->bytes >> 8, branch_operand->bytes & 0xff);
                bytes_printed += 2;
            } else {
                glk_printf(" %02x", branch_operand->bytes >> 8);
                bytes_printed++;
            }
        }
        for (bytes_printed = 36 -(bytes_printed * 3); bytes_printed > 0; bytes_printed--)
            glk_put_string(" ");
    }

    glk_printf(" %-16s", opcode_name(buf, instruction->count, instruction->opcode));

    // if (instruction->count != COUNT_0OP)
    //     print_operand_list(operands);

    /* some instructions have special formats for their opcodes, otherwise just list them */
    switch (instruction->count) {
        case COUNT_2OP:
            switch (instruction->opcode) {
                case STORE:
                    print_variable(operands->bytes, 0);
                    glk_put_string(",");
                    print_operand(operands + 1);
                    break;
                default: print_operand_list(operands); break;
            }
            break;
        case COUNT_1OP:
            switch (instruction->opcode) {
                default: print_operand_list(operands); break;
            }
            break;
        case COUNT_0OP:
            switch (instruction->opcode) {
                case PRINT:
                case PRINT_RET:
                    glk_put_string("\"");
                    print_zstring(zPC);
                    glk_put_string("\"\n");
                    break;
                default: break;
            }
            break;
        case COUNT_VAR:
            switch (instruction->opcode) {
                case CALL:
                    glk_printf("%x", unpack(operands->bytes));
                    if ((operands + 1)->type != NONE) {
                        glk_put_string(" (");
                        print_operand_list(operands + 1);
                        glk_put_string(")");
                    }
                    break;
                default: print_operand_list(operands); break;
            }
            break;
    }
    
    if (instruction->store_flag){
        glk_put_string(" -> ");
        print_variable(*store_operand, PRINT_STORE);
    }

    if (instruction->branch_flag) {
        glk_printf(" [%s]", branch_operand->test ? "TRUE" : "FALSE");
        if (branch_operand->offset == 0 || branch_operand->offset == 1) {
            branch_operand->offset ? glk_put_string(" RTRUE") : glk_put_string(" RFALSE");
        } else {
            glk_printf(" %04x", (zPC + branch_operand->offset) - 2);            
        }
    }

}

static void print_operand(zoperand_t *op_ptr) {
    switch (op_ptr->type) {
        case LARGE_CONST: glk_printf("#%04x", op_ptr->bytes); break;
        case SMALL_CONST: glk_printf("#%02x", op_ptr->bytes); break;
        case VARIABLE: print_variable(op_ptr->bytes, PRINT_READ); break;
    }
}

static void print_operand_list(zoperand_t *op_ptr) {
    while(op_ptr->type != NONE) {
        print_operand(op_ptr);
        op_ptr++;
        if (op_ptr->type != NONE)
            glk_put_string(",");
    }
}

static void print_variable(zbyte_t number, int flags) {
    if (number == 0) {
        flags & PRINT_READ ? glk_printf("(SP)+") : glk_printf("-(SP)");
    } else if (number > 0 && number < 0x10) {
        glk_printf("L%02x", number - 1);
    } else {
        glk_printf("G%02x", number - 0x10);
    }
}

static char * opcode_name(char *buf, zbyte_t opcount, zbyte_t opcode) {
    switch (opcount) {
        case COUNT_2OP:
            switch (opcode) {
                case JE: strcpy(buf, "JE"); break;
                case JL: strcpy(buf, "JL"); break;
                case JG: strcpy(buf, "JG"); break;
                case DEC_CHK: strcpy(buf, "DEC_CHK"); break;
                case INC_CHK: strcpy(buf, "INC_CHK"); break;
                case JIN: strcpy(buf, "JIN"); break;
                case TEST: strcpy(buf, "TEST"); break;
                case OR: strcpy(buf, "OR"); break;
                case AND: strcpy(buf, "AND"); break;
                case TEST_ATTR: strcpy(buf, "TEST_ATTR"); break;
                case SET_ATTR: strcpy(buf, "SET_ATTR"); break;
                case CLEAR_ATTR: strcpy(buf, "CLEAR_ATTR"); break;
                case STORE: strcpy(buf, "STORE"); break;
                case INSERT_OBJ: strcpy(buf, "INSERT_OBJ"); break;
                case LOADW: strcpy(buf, "LOADW"); break;
                case LOADB: strcpy(buf, "LOADB"); break;
                case GET_PROP: strcpy(buf, "GET_PROP"); break;
                case GET_PROP_ADDR: strcpy(buf, "GET_PROP_ADDR"); break;
                case GET_NEXT_PROP: strcpy(buf, "GET_NEXT_PROP"); break;
                case ADD: strcpy(buf, "ADD"); break;
                case SUB: strcpy(buf, "SUB"); break;
                case MUL: strcpy(buf, "MUL"); break;
                case DIV: strcpy(buf, "DIV"); break;
                case MOD: strcpy(buf, "MOD"); break;
            }
            break;
        case COUNT_1OP:
            switch (opcode) {
                case JZ: strcpy(buf, "JZ"); break;
                case GET_SIBLING: strcpy(buf, "GET_SIBLING"); break;
                case GET_CHILD: strcpy(buf, "GET_CHILD"); break;
                case GET_PARENT: strcpy(buf, "GET_PARENT"); break;
                case GET_PROP_LEN: strcpy(buf, "GET_PROP_LEN"); break;
                case INC: strcpy(buf, "INC"); break;
                case DEC: strcpy(buf, "DEC"); break;
                case PRINT_ADDR: strcpy(buf, "PRINT_ADDR"); break;
                case REMOVE_OBJ: strcpy(buf, "REMOVE_OBJ"); break;
                case PRINT_OBJ: strcpy(buf, "PRINT_OBJ"); break;
                case RET: strcpy(buf, "RET"); break;
                case JUMP: strcpy(buf, "JUMP"); break;
                case PRINT_PADDR: strcpy(buf, "PRINT_PADDR"); break;
                case LOAD: strcpy(buf, "LOAD"); break;
                case NOT: strcpy(buf, "NOT"); break;
            }
            break;
        case COUNT_0OP:
            switch (opcode) {
                case RTRUE: strcpy(buf, "RTRUE"); break;
                case RFALSE: strcpy(buf, "RFALSE"); break;
                case PRINT: strcpy(buf, "PRINT"); break;
                case PRINT_RET: strcpy(buf, "PRINT_RET"); break;
                case NOP: strcpy(buf, "NOP"); break;
                case SAVE: strcpy(buf, "SAVE"); break;
                case RESTORE: strcpy(buf, "RESTORE"); break;
                case RESTART: strcpy(buf, "RESTART"); break;
                case RET_POPPED: strcpy(buf, "RET_POPPED"); break;
                case POP: strcpy(buf, "POP"); break;
                case QUIT: strcpy(buf, "QUIT"); break;
                case NEW_LINE: strcpy(buf, "NEW_LINE"); break;
                case SHOW_STATUS: strcpy(buf, "SHOW_STATUS"); break;
                case VERIFY: strcpy(buf, "VERIFY"); break;
            }
            break;
        case COUNT_VAR:
            switch (opcode) {
                case CALL: strcpy(buf, "CALL"); break;
                case STOREW: strcpy(buf, "STOREW"); break;
                case STOREB: strcpy(buf, "STOREB"); break;
                case PUT_PROP: strcpy(buf, "PUT_PROP"); break;
                case SREAD: strcpy(buf, "SREAD"); break;
                case PRINT_CHAR: strcpy(buf, "PRINT_CHAR"); break;
                case PRINT_NUM: strcpy(buf, "PRINT_NUM"); break;
                case RANDOM: strcpy(buf, "RANDOM"); break;
                case PUSH: strcpy(buf, "PUSH"); break;
                case PULL: strcpy(buf, "PULL"); break;
                case SPLIT_WINDOW: strcpy(buf, "SPLIT_WINDOW"); break;
                case SET_WINDOW: strcpy(buf, "SET_WINDOW"); break;
                case OUTPUT_STREAM: strcpy(buf, "OUTPUT_STREAM"); break;
                case INPUT_STREAM: strcpy(buf, "INPUT_STREAM"); break;
                case SOUND_EFFECT: strcpy(buf, "SOUND_EFFECT"); break;
            }
            break;
        default:
            strcpy(buf, "UNKNOWN"); break;
    }
    return buf;
}