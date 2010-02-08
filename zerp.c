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
#include "parse.h"
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
zword_t zDictionaryHeader = 0;
zword_t zDictionary = 0;
packed_addr_t zPC = 0;
packed_addr_t instructionPC = 0;

static int test_je(zword_t value, zoperand_t *operands);

/* main interpreter entrypoint */
int zerp_run() {
    zinstruction_t instruction;
    zoperand_t operands[8];
    zbranch_t branch_operand;
    zword_t store_operand, scratch1, scratch2, scratch3, scratch4;
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
	zDictionaryHeader = get_word(DICTIONARY);
	zDictionary = zDictionaryHeader + 4 + get_byte(zDictionaryHeader);

    set_header_flags();

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
        // if (zPC == 0x5d00)
        // 	        debug_monitor(instructionPC, instruction, *operands, store_operand, branch_operand);

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
                        scratch1 = get_operand(0);
						scratch4 = get_operand(1);
                        if (scratch1 == 0) {
                            scratch3 = (signed short)stack_peek() - 1;
                            stack_poke(scratch3);
                        } else {
                            scratch2 = variable_get(scratch1);
                            scratch3 = variable_set(scratch1, (signed short)scratch2 - 1);
                        }
                        branch_op((signed short)scratch3 < (signed short)scratch4);
                        break;
                    case INC_CHK:
                        scratch1 = get_operand(0);
						scratch4 = get_operand(1);
                        if (scratch1 == 0) {
                            scratch3 = (signed short)stack_peek() + 1;
                            stack_poke(scratch3);
                        } else {
                            scratch2 = variable_get(scratch1);
                            scratch3 = variable_set(scratch1, (signed short)scratch2 + 1);
                        }
                        branch_op((signed short)scratch3 > (signed short)scratch4);
                        break;
                    case JIN:
                        branch_op(object_in(get_operand(0), get_operand(1)))
                        break;
                    case TEST:
						scratch1 = get_operand(0);
						scratch2 = get_operand(1);
                        branch_op((scratch1 & scratch2) == scratch2)
                        break;
                    case OR:
                        store_op(get_operand(0) | get_operand(1))
                        break;
                    case AND:
                        store_op(get_operand(0) & get_operand(1))
                        break;
                    case TEST_ATTR:
                        branch_op(get_attribute(get_operand(0), get_operand(1)))
                        break;
                    case SET_ATTR:
                        set_attribute(get_operand(0), get_operand(1));
                        break;
                    case CLEAR_ATTR:
                        clear_attribute(get_operand(0), get_operand(1));
                        break;
                    case STORE:
                        scratch1 = get_operand(0);
                        scratch2 = get_operand(1);
                        indirect_variable_set(scratch1, scratch2);
                        break;
                    case INSERT_OBJ:
                        insert_object(get_operand(0), get_operand(1));
                        break;
                    case LOADW:
						scratch1 = get_operand(0);
						scratch2 = get_operand(1);
						LOG(ZDEBUG, "\nLoading word at #%x", scratch1 + scratch2 * 2)
                        store_op(get_word(scratch1 + scratch2 * 2))
                        break;
                    case LOADB:
						scratch1 = get_operand(0);
						scratch2 = get_operand(1);
						LOG(ZDEBUG, "\nLoading byte at #%x", scratch1 + scratch2)
                        store_op(get_byte(scratch1 + scratch2))
                        break;
                    case GET_PROP:
                        store_op(get_property(get_operand(0), get_operand(1)))
                        break;
                    case GET_PROP_ADDR:
                        store_op(get_property_address(get_operand(0), get_operand(1)))
                        break;
                    case GET_NEXT_PROP:
                        store_op(get_next_property(get_operand(0), get_operand(1)))
                        break;
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
                    default:
                        LOG(ZERROR, "Unknown opcode: %#04x", instruction.bytes);
                        fatal_error("bad op code.");
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
                    case GET_PROP_LEN:
                        store_op(get_property_length(get_operand(0)))
                        break;
                    case INC:
                        scratch1 = get_operand(0);
                        if (scratch1 == 0) {
                            stack_poke((signed short)stack_peek() + 1);
                        } else {
                            scratch2 = variable_get(scratch1);
                            variable_set(scratch1, (signed short)scratch2 + 1);
                        }
                        break;
                    case DEC:
                        scratch1 = get_operand(0);
                        if (scratch1 == 0) {
                            stack_poke((signed short)stack_peek() - 1);
                        } else {
                            scratch2 = variable_get(scratch1);
                            variable_set(scratch1, (signed short)scratch2 - 1);
                        }
                        break;
                    case PRINT_ADDR:
                        print_zstring(get_operand(0));
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
                        zPC += (signed short) (get_operand(0) - 2);
                        break;
                    case PRINT_PADDR:
                        print_zstring(unpack(get_operand(0)));
                        break;
                    case LOAD:
                        if (operands[0].type == VARIABLE) {
                            store_op(indirect_variable_get(get_operand(0)))
                        } else {
                            store_op(indirect_variable_get(operands[0].bytes))
                        }
                        break;
                    case NOT:
                        store_op(~get_operand(0))
                        break;
                    default:
                        LOG(ZERROR, "Unknown opcode: %#04x", instruction.bytes);
                        fatal_error("bad op code.");
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
                    case SAVE:
                        branch_op(1)
                        break;
                    case RESTORE:
                        branch_op(1)
                        break;
                    case RESTART:
                        break;
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
                    case SHOW_STATUS:
                        break;
                    case VERIFY:
                        branch_op(1)
                        break;
                    default:
                        LOG(ZERROR, "Unknown opcode: %#04x", instruction.bytes);
                        fatal_error("bad op code.");
                }
                break;
            case COUNT_VAR:
                switch (instruction.opcode) {
                    case CALL:
                        call_zroutine(unpack(get_operand(0)), &operands[1], store_operand);
                        break;
                    case STOREW:
                        scratch1 = get_operand(0); scratch2 = get_operand(1); scratch3 = get_operand(2);
                        LOG(ZDEBUG, "\nStoring word value %i at #%x", scratch3, scratch1 + scratch2 * 2)
                        store_word(scratch1 + scratch2 * 2, scratch3);
                        break;
                    case STOREB:
                        scratch1 = get_operand(0); scratch2 = get_operand(1); scratch3 = get_operand(2);
                        LOG(ZDEBUG, "\nStoring byte value %i at #%x", scratch3, scratch1 + scratch2)
                        store_byte(scratch1 + scratch2, scratch3)
                        break;
                    case PUT_PROP:
                        put_property(get_operand(0), get_operand(1), get_operand(2));
                        break;
                    case SREAD:
                        read(get_operand(0), get_operand(1));
                        break;
                    case PRINT_CHAR:
                        glk_put_char(get_operand(0));
                        break;
                    case PRINT_NUM:
                        glk_printf("%d", (signed short)get_operand(0));
                        break;
                    case RANDOM:
						scratch1 = (signed short) get_operand(0);
                        if (scratch1 < (zword_t) 0) {
                            srandom((unsigned short)scratch1);
                            variable_set(store_operand, 0);
                        } else if (scratch1 == 0) {
                            srandom(time(0));
                            variable_set(store_operand, 0);
						} else {
							scratch2 = (random() % scratch1) + 1;
                            variable_set(store_operand, scratch2);
                        }
                        break;
                    case PUSH:
                        stack_push(get_operand(0));
                        break;
                    case PULL:
                        scratch1 = get_operand(0);
                        indirect_variable_set(scratch1, stack_pop());
                        break;
                    case SPLIT_WINDOW:
                    case SET_WINDOW:
                    case OUTPUT_STREAM:
                    case INPUT_STREAM:
                    case SOUND_EFFECT:
                        break;
                    default:
                        LOG(ZERROR, "Unknown opcode: %#04x", instruction.bytes);
                        fatal_error("bad op code.");
                }
                break;
            default:
                LOG(ZERROR, "Unknown opcode: %#04x", instruction.bytes);
                fatal_error("bad opcode");
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

static void set_header_flags() {
    store_byte(TERP_NUMBER, 3);
    store_byte(TERP_VERSION, '0');
}