/* 
    Zerp: a Z-machine interpreter
    zerp.c : main z-machine interpreter
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TARGET_OS_MAC
#include <GlkClient/glk.h>
#else
#include "glk.h"
#endif /* TARGET_OS_MAC */
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
int zPackedShift = 0;

static int test_je(zword_t value, zoperand_t *operands);

/* main interpreter entrypoint */
int zerp_run() {
    zinstruction_t instruction;
    zoperand_t operands[9]; /* 9th op will hold the end of list marker for 8 op opcodes */
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
    zObjects = zProperties + (zGameVersion > Z_VERSION_3 ? 126 : 62);
	zDictionaryHeader = get_word(DICTIONARY);
	zDictionary = zDictionaryHeader + 4 + get_byte(zDictionaryHeader);
	switch (zGameVersion) {
			case Z_VERSION_3:
				zPackedShift = 1;
				break;
			case Z_VERSION_8:
				zPackedShift = 3;
				break;
			default:
				zPackedShift = 2;
				break;
	}
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

       // if (zPC >= 0xb2d5 && zPC <= 0xb31c)
       // 	       print_zinstruction(instructionPC, &instruction, operands, &store_operand, &branch_operand, 0);
       // if (zPC >= 0xb2d5 && zPC <= 0xb31c)
       //    	        debug_monitor(instructionPC, instruction, *operands, store_operand, branch_operand);

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
					case CALL_2S:
							call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, TRUE);
							break;
					case CALL_2N:
							call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, FALSE);
							break;
					case SET_COLOUR:
							unimplemented("SET_COLOUR")
							break;
					case THROW:
							unimplemented("THROW")
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
					case CALL_1S:
						call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, TRUE);
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
						if (zGameVersion <= Z_VERSION_4) {
	                        store_op(~get_operand(0))
						} else {
							call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, FALSE);
						}
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
						if (zGameVersion < Z_VERSION_3) {
	                        branch_op(1)
						} else if (zGameVersion == Z_VERSION_4) {
							store_op(1)
						} else {
	                        fatal_error("SAVE illegal in > V4");
						}
                        break;
                    case RESTORE:
						if (zGameVersion < Z_VERSION_3) {
		                      branch_op(1)
						} else if (zGameVersion == Z_VERSION_4) {
							store_op(1)
						} else {
	                      fatal_error("RESTORE illegal in > V4");
						}
                        break;
                    case RESTART:
                        break;
                    case RET_POPPED:
                        return_zroutine(stack_pop());
                        break;
                    case POP:
						if (zGameVersion >= Z_VERSION_5) {
							unimplemented("CATCH");
						} else {
							stack_pop();
						}
                        break;
                    case QUIT:
                        running = FALSE;
                        break;
                    case NEW_LINE:
                        glk_put_string("\n");
                        break;
                    case SHOW_STATUS:
						if (zGameVersion < Z_VERSION_4) {
							show_status_line();
						} else {
							fatal_error("SHOW_STATUS illegal in > V3");
						}
                        break;
					case PIRACY:
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
                        call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, TRUE);
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
						/* TODO: Timed input */
						if (zGameVersion <= Z_VERSION_4) {
							show_status_line();
	                        read(get_operand(0), get_operand(1));
						} else if (zGameVersion == Z_VERSION_4) {
	                        read(get_operand(0), get_operand(1));
						} else {
							store_op(read(get_operand(0), get_operand(1)))
						}
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
						// glk_printf("SPLIT_WINDOW %d", get_operand(0));
						if (scratch1 = get_operand(0)) {
							upperwin = glk_window_open(mainwin, winmethod_Above | winmethod_Fixed, scratch1, wintype_TextGrid, 0);
							set_screen_width(upperwin);
						} else {
							glk_window_close(upperwin, 0);
							upperwin = 0;
						}
						break;
                    case SET_WINDOW:
						// glk_printf("SET_WINDOW %d", get_operand(0));
						if (!get_operand(0)) {
							glk_set_window(mainwin);
						} else {
							if (upperwin) {
								glk_set_window(upperwin);
								set_screen_width(upperwin);
							}
						}
						break;
					case CALL_VS2:
						call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, TRUE);
						break;
					case ERASE_WINDOW:
						// glk_printf("ERASE_WINDOW %d", get_operand(0));
						switch ((signed short) get_operand(0)) {
							case 0:
								glk_window_clear(mainwin);
								break;
							case 1:
								glk_window_clear(upperwin);
								break;
							case -1:
								if (upperwin)
									glk_window_close(upperwin, 0);
								glk_window_clear(mainwin);
								break;
							case -2:
								if (upperwin)
									glk_window_clear(upperwin);
								glk_window_clear(mainwin);
								break;
						}
						break;
					case ERASE_LINE:
						break;
					case SET_CURSOR:
						// glk_printf("SET_CURSOR %d %d", get_operand(1) - 1, get_operand(0) - 1);
						if (upperwin)
							glk_window_move_cursor(upperwin, get_operand(1) - 1, get_operand(0) - 1);
						break;
					case GET_CURSOR:
					case SET_TEXT_STYLE:
						scratch1 = get_operand(0);
						if (!scratch1) {
							glk_set_style(style_Normal);
							break;
						}
						scratch2 = 0;
						if (scratch1 & 1)
							scratch2 |= style_Alert;
						if (scratch1 & 2)
							scratch2 |= style_Emphasized;
						if (scratch1 & 4)
							scratch2 |= style_Emphasized;
						if (scratch1 & 8)
							scratch2 |= style_Preformatted;
						glk_set_style(scratch2);
						break;
					case BUFFER_MODE:
                    case OUTPUT_STREAM:
                    case INPUT_STREAM:
                    case SOUND_EFFECT:
                        break;
					case READ_CHAR:
						/* TODO: Timed input */
						store_op(read_char(1))
						break;
					case SCAN_TABLE:
						if (operands[3].type == NONE) {
							scratch1 = 0x82; /* compare words, 2 byte table entries is the default */
						} else {
							scratch1 = get_operand(3);
						}
						scratch2 = scan_table(get_operand(0), get_operand(1), get_operand(2), scratch1);
						store_op(scratch2);
						branch_op(scratch2 != 0);
						break;
					case NOT_V5:
                        store_op(~get_operand(0))
						break;
					case CALL_VN:
						call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, FALSE);
						break;
					case CALL_VN2:
						call_zroutine(unpack(get_operand(0)), &operands[1], store_operand, FALSE);
						break;
					case TOKENISE:
						tokenise(get_operand(0), get_operand(1), 0, 0);
						break;
					case ENCODE_TEXT:
						unimplemented("ENCODE_TEXT")
						break;
					case COPY_TABLE:
						unimplemented("COPY_TABLE")
						break;
					case PRINT_TABLE:
						unimplemented("PRINT_TABLE")
						break;
					case CHECK_ARG_COUNT:
						scratch1 = get_operand(0) - 1;
						scratch2 = zFP->args >> scratch1;
						scratch3 = scratch2 & 1;
						branch_op(scratch3)
						// branch_op(((zFP->args >> (get_operand(0) - 1)) & 1))
						break;
                    default:
                        LOG(ZERROR, "Unknown opcode: %#04x", instruction.bytes);
                        fatal_error("bad op code.");
                }
                break;
			case COUNT_EXT:
				switch (instruction.opcode) {
					case SAVE_TABLE:
						unimplemented("SAVE_TABLE");
						break;
					case RESTORE_TABLE:
						unimplemented("RESTORE_TABLE");
						break;
					case LOG_SHIFT:
						scratch1 = get_operand(0);
						scratch2 = get_operand(1);
						if ((signed short)scratch2 < 0) {
							store_op(scratch1 >> -(signed)scratch2)
						} else {
							store_op(scratch1 << scratch2)						
						}
						break;
					case ART_SHIFT:
						scratch1 = get_operand(0);
						scratch2 = get_operand(1);
						if ((signed short)scratch2 < 0) {
							store_op((signed short)scratch1 >> -(signed short)scratch2)
						} else {
							store_op(scratch1 << scratch2)
						}
						break;
					case SET_FONT:
					case SAVE_UNDO:
					case RESTORE_UNDO:
					case PRINT_UNICODE:
					case CHECK_UNICODE:
						break;
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

static zword_t scan_table(zword_t item, zword_t table, zword_t length, zbyte_t form) {
	int i;
	zword_t address;

	address = 0;
	for (i = 0; i < length; i++) {
		if (form & 0x80) {
			if (get_word(table) == item) {
				address = table;
				break;
			}
		} else {
			if (get_byte(table) == item) {
				address = table;
				break;
			}
		}
		table += (form & 0x7f);
	}

	return address;
}


static void set_header_flags() {
    store_byte(TERP_NUMBER, 3);
    store_byte(TERP_VERSION, '0');

	/* FIXME: quick hack - fake a 80x24 screen for Infocom later games, we will update when we open a textgrid window */
	store_byte(SCREEN_HEIGHT, 24);
	set_screen_width(mainwin);
}
