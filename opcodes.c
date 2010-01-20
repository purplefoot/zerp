/* 
    Zerp: a Z-machine interpreter
    opcodes.c : opcode decoding
*/

#include <stdio.h>
#include <stdlib.h>
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
        instruction->size = OP_VARIABLE;
        LOG(ZERROR, "Unsupported extended opcode %#02x @ %#04x\n", get_byte(pc++));
        decode_variable(&pc, instruction, get_byte(pc++), operands);
    } else if (instruction->bytes >> 6 == OP_VARIABLE ) {
        /*
            4.3.3
            In variable form, if bit 5 is 0 then the count is 2OP; if it is 1, then the count is
            VAR. The opcode number is given in the bottom 5 bits.  
        */
        instruction->size = OP_VARIABLE;
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
        instruction->size = OP_SHORT;
        instruction->count = (instruction->bytes >> 4) & 0x03 == 0x03 ? COUNT_0OP : COUNT_1OP;
        instruction->opcode = instruction->bytes & OPCODE_4BIT;
        if (instruction->count = COUNT_1OP)
            decode_short(&pc, instruction, operands);
    } else { /* OP_LONG */
        /*
            4.3.2
            In long form the operand count is always 2OP. The opcode number is given in the
            bottom 5 bits.
        */
        instruction->size = OP_LONG;
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
    switch ((instruction->bytes >> 4) & 0x3) {
        case LARGE_CONST:
            (operands++)->bytes = get_word((*pc)++);
            (*pc)++;
            break;
        case SMALL_CONST:
            (operands++)->bytes = get_word((*pc)++);
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
    branch->flag = branch_short >> 7 & 1;
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