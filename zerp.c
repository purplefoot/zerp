/* 
    Zerp: a Z-machine interpreter
    zerp.c : main z-machine interpreter
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glk.h"
#include "zerp.h"
#include "opcodes.h"
#include "stack.h"
#include "objects.h"
#include "debug.h"

zword_t * zStack = 0;
zword_t * zSP = 0;
zword_t * zStackTop = 0;
zstack_frame_t * zCallStack = 0;
zstack_frame_t * zFP = 0;
zstack_frame_t * zCallStackTop = 0;
zword_t zGlobals = 0;
zword_t zProperties = 0;
zword_t zObjects = 0;
packed_addr_t zPC = 0;

static int test_je(zword_t value, zoperand_t *operands);

/* main interpreter entrypoint */
int zerp_run() {
    zinstruction_t instruction;
    zoperand_t operands[8];
    zbranch_t branch_operand;
    zword_t store_operand, scratch1, scratch2, scratch3;
    packed_addr_t instructionPC;
    int running;


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
    zProperties = get_word(OBJECT_TABLE);
    zObjects = zProperties + 62;
    
    running = TRUE;
    LOG(ZDEBUG,"Running...\n", 0);
    
    while (running) {
        instructionPC = zPC;
        zword_t res;
        
        memset(&instruction, 0, sizeof(zinstruction_t));
        memset(&operands, 0, sizeof(zoperand_t) * 8);
        memset(&branch_operand, 0, sizeof(zbranch_t));
        
        zPC += decode_instruction(zPC, &instruction, operands, &store_operand, &branch_operand);

        // print_zinstruction(instructionPC, &instruction, operands, &store_operand, &branch_operand, 0);
        // debug_monitor(instructionPC, instruction, *operands, store_operand, branch_operand);

        switch (instruction.count) {
            case COUNT_2OP:
                switch (instruction.opcode) {
                    case JE:
                        branch_op(test_je(get_operand(0), &operands[1]))
                        break;
                    case JL:
                        branch_op((signed short)get_operand(0) < (signed short)get_operand(1))
                        break;
                    case JG:
                        branch_op((signed short)get_operand(0) > (signed short)get_operand(1))
                        break;
                    case DEC_CHK:
                        branch_op(variable_set(operands[0].bytes, variable_get(operands[0].bytes) - 1) < operands[1].bytes)
                        break;
                    case INC_CHK:
                        branch_op(variable_set(operands[0].bytes, variable_get(operands[0].bytes) + 1) > operands[1].bytes)
                        break;
                    case JIN:
                        branch_op(object_in(get_operand(0), get_operand(1)))
                        break;
                    case TEST:
                        branch_op(get_operand(0) & get_operand(1) == get_operand(1))
                        break;
                    case OR:
                        store_op(get_operand(0) | get_operand(1))
                        break;
                    case AND:
                        store_op(get_operand(0) & get_operand(1))
                        break;
        //             case TEST_ATTR:
        //                 branch_op("#TEST_ATTR %#s, %#s, ", 0)
        //                 break;
        //             case SET_ATTR:
        //                 LOG(ZDEBUG, "#SET_ATTR %#s, %#s\n", opdesc[0], opdesc[1]);
        //                 break;
        //             case CLEAR_ATTR:
        //                 LOG(ZDEBUG, "#CLEAR_ATTR %#s, %#s\n", opdesc[0], opdesc[1]);
        //                 break;
                    case STORE:
                        variable_set(operands[0].bytes, get_operand(1));
                        break;
        //             case INSERT_OBJ:
        //                 LOG(ZDEBUG, "#INSERT_OBJ %#s, %#s\n", opdesc[0], opdesc[1]);
        //                 break;
                    case LOADW:
                        store_op(get_word(get_operand(0) + get_operand(1) * 2))
                        break;
                    case LOADB:
                        store_op(get_byte(get_operand(0) + get_operand(1)))
                        break;
        //             case GET_PROP:
        //                 store_op("#GET_PROP %#s, %#s", 0xbeef)
        //                 break;
        //             case GET_PROP_ADDR:
        //                 store_op("#GET_PROP_ADDR %#s, %#s", 0xbeef)
        //                 break;
        //             case GET_NEXT_PROP:
        //                 store_op("#GET_NEXT_PROP %#s, %#s", 0xbeef)
        //                 break;
                    case ADD:
                        store_op((signed short)get_operand(0) + (signed short)get_operand(1))
                        break;
                    case SUB:
                        store_op((signed short)get_operand(0) - (signed short)get_operand(1))
                        break;
                    case MUL:
                        store_op((signed short)get_operand(0) * (signed short)get_operand(1))
                        break;
                    case DIV:
                        store_op((signed short)get_operand(0) / (signed short)get_operand(1))
                        break;
                    case MOD:
                        store_op((signed short)get_operand(0) % (signed short)get_operand(1))
                        break;
                }
                break;
            case COUNT_1OP:
                switch (instruction.opcode) {
                    case JZ:
                        branch_op(get_operand(0) == 0)
                        break;
                    case GET_SIBLING:
                        scratch1 = object_sibling(get_operand(0));
                        store_op(scratch1)
                        branch_op(scratch1 != 0)
                        break;
                    case GET_CHILD:
                        scratch1 = object_child(get_operand(0));
                        store_op(scratch1)
                        branch_op(scratch1 != 0)
                        break;
                    case GET_PARENT:
                        store_op(object_parent(get_operand(0)))
                        break;
        //             case GET_PROP_LEN:
        //                 store_op("#GET_PROP_LEN %#s", 0xbeef)
        //                 break;
                    case INC:
                        variable_set(operands[0].bytes, ((signed short)variable_get(operands[0].bytes)) + 1);
                        break;
                    case DEC:
                        variable_set(operands[0].bytes, ((signed short)variable_get(operands[0].bytes)) - 1);
                        break;
                    case PRINT_ADDR:
                        print_zstring(operands[0].bytes);
                        break;
                    case REMOVE_OBJ:
                        remove_object(get_operand(0));
                        break;
                    case PRINT_OBJ:
                        print_object_name(get_operand(0));
                        break;
                    case RET:
                        return_zroutine(get_operand(0));
                        break;
                    case JUMP:
                        zPC += (signed short) (operands[0].bytes - 2);
                        break;
                    case PRINT_PADDR:
                        print_zstring(unpack(operands[0].bytes));
                        break;
                    case LOAD:
                        store_op(variable_get(operands[0].bytes))
                        break;
                    case NOT:
                        store_op(~get_operand(0))
                        break;
                }
                break;
            case COUNT_0OP:
                switch (instruction.opcode) {
                    case RTRUE:
                        return_zroutine(1);
                        break;
                    case RFALSE:
                        return_zroutine(0);
                        break;
                    case PRINT:
                        zPC += print_zstring(zPC);
                        break;
                    case PRINT_RET:
                        zPC += print_zstring(zPC);
                        glk_put_string("\n");
                        return_zroutine(1);
                        break;
                    case NOP:
                        break;
        //             case SAVE:
        //                 branch_op("#SAVE", 1)
        //                 break;
        //             case RESTORE:
        //                 branch_op("#RESTORE", 1)
        //                 break;
        //             case RESTART:
        //                 LOG(ZDEBUG, "#RESTART\n", 0);
        //                 break;
                    case RET_POPPED:
                        return_zroutine(stack_pop());
                        break;
                    case POP:
                        stack_pop();
                        break;
                    case QUIT:
                        running = FALSE;
                        break;
                    case NEW_LINE:
                        glk_put_string("\n");
                        break;
                    // case SHOW_STATUS:
                    //     break;
        //             case VERIFY:
        //                 branch_op("#VERIFY", 1)
        //                 break;
                }
                break;
            case COUNT_VAR:
                switch (instruction.opcode) {
                    case CALL:
                        call_zroutine(unpack(operands[0].bytes), &operands[1], store_operand);
                        break;
                    case STOREW:
                        scratch1 = get_operand(0); scratch2 = get_operand(1); scratch3 = get_operand(2);
                        store_word(scratch1 + scratch2 * 2, scratch3);
                        LOG(ZDEBUG, "STOREW %04x -> %04x\n", scratch1 + scratch2 * 2, scratch3)
                        break;
                    case STOREB:
                        scratch1 = get_operand(0); scratch2 = get_operand(1); scratch3 = get_operand(2);
                        store_byte(scratch1 + scratch2, scratch3)
                        LOG(ZDEBUG, "STOREB %04x -> %04x\n", scratch1 + scratch2, scratch3)
                        break;
        //             case PUT_PROP:
        //                 LOG(ZDEBUG, "#PUT_PROP %#s %#s -> %#s \n", opdesc[0], opdesc[1], opdesc[2]);
        //                 break;
                    case SREAD:
                        glk_put_string("INPUT NOW REQUIRED");
                        break;
                    case PRINT_CHAR:
                        glk_put_char(operands[0].bytes);
                        break;
                    case PRINT_NUM:
                        glk_printf("%d", (signed short)operands[0].bytes);
                        break;
                    case RANDOM:
                        if ((signed short)operands[0].bytes < 0) {
                            srandom(operands[0].bytes);
                            variable_set(store_operand, 0);
                        } else {
                            variable_set(store_operand, random());
                        }
                        break;
                    case PUSH:
                        stack_push(operands[0].bytes);
                        break;
                    case PULL:
                        variable_set(operands[0].bytes, stack_pop);
                        break;
        //             case SPLIT_WINDOW:
        //                 LOG(ZDEBUG, "#SPLIT_WINDOW %#s\n", opdesc[0]);
        //                 break;
        //             case SET_WINDOW:
        //                 LOG(ZDEBUG, "#SET_WINDOW %#s\n", opdesc[0]);
        //                 break;
        //             case OUTPUT_STREAM:
        //                 LOG(ZDEBUG, "#OUTPUT_STREAM %#s\n", opdesc[0]);
        //                 break;
        //             case INPUT_STREAM:
        //                 LOG(ZDEBUG, "#INPUT_STREAM %#s\n", opdesc[0]);
        //                 break;
        //             case SOUND_EFFECT:
        //                 LOG(ZDEBUG, "#SOUND_EFFECT %#s\n", opdesc[0]);
        //                 break;
                }
                break;
            default:
                LOG(ZERROR, "Unknown opcode: %#x", instruction.opcode);
                fatal_error("Unknown Z-machine opcode.");
        }
    }
    
    /* Done, so clean up */
    free(zStack);
    free(zCallStack);
}

static int test_je(zword_t value, zoperand_t *operands) {
    int result = FALSE;
    
    while (!result && operands->type != NONE) {
        result = (get_operand_ptr(operands) == value);
        operands++;        
    }
        
    return result;
}