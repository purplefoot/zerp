/* 
    Zerp: a Z-machine interpreter
    zerp.h : z-machine main interface
*/

#ifndef ZERP_H
#define ZERP_H

/* We define our own TRUE and FALSE and NULL, because ANSI
    is a strange world. */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

/* Debugging */
#define ZERROR          1
#define ZWARN           2
#define ZDEBUG          3
#define ZCRAZY          4

#define DEBUG           1

#ifdef DEBUG
#define LOG(level, fmt, ...) \
    if (DEBUG >= level) { \
		char dbuff[256]; \
		sprintf(dbuff, fmt, __VA_ARGS__); \
		fprintf(stderr, "%5x %s\n", instructionPC, dbuff + 1); \
	}
#else
#define LOG(level, fmt, ...)
#endif


#define SMALLBUFF 1024

/* Z-machine address formats */
typedef unsigned short zword_t;
typedef unsigned char zbyte_t;
typedef unsigned int packed_addr_t;

#define get_byte(offset) *(zMachine + offset)
#define store_byte(offset, value) get_byte(offset) = (zbyte_t) value;
#define get_word(addr) ((zword_t) (get_byte(addr) << 8 | get_byte(addr + 1)))
#define store_word(offset, value) store_byte(offset, (zbyte_t)(value >> 8)); store_byte(offset + 1, (zbyte_t)(value & 0xff));
#define get_word_addr(addr) get_word(addr) >> 1
#define store_word_addr(addr) store_word(addr << 1)
#define get_packed_addr(addr) get_word(addr) << 1
#define store_packed_addr(addr) store_word(addr >> 1)
#define unpack(addr) addr << zPackedShift

typedef struct zstack_frame {
    packed_addr_t pc;
    zword_t *sp;
    zbyte_t ret_store;
	zbyte_t ret_keep;
    zword_t locals[16];
	zbyte_t args;
} zstack_frame_t;

/* game file */
extern frefid_t zGamefileRef;
extern int zFilesize;
extern char * zFilename;
extern unsigned char * zGamefile;
extern unsigned char * zMachine;
extern int zGameVersion;
extern int zPackedShift;

/* z-machine stack */
extern zword_t * zStack;
extern zword_t * zSP;
extern zword_t * zStackTop;

extern zstack_frame_t * zCallStack;
extern zstack_frame_t * zFP;
extern zstack_frame_t * zCallStackTop;

#define STACKSIZE 8192
#define CALLSTACKSIZE 512

extern zword_t zGlobals;
extern zword_t zProperties;
extern zword_t zObjects;
extern zword_t zDictionaryHeader;
extern zword_t zDictionary;

extern packed_addr_t zPC;
extern packed_addr_t instructionPC;

/* header offsets */
#define Z_VERSION           0x00
#define FLAGS_1             0x01
#define HIGH_MEM            0x04
#define PC_INITIAL          0x06
#define DICTIONARY          0x08
#define OBJECT_TABLE        0x0a
#define GLOBALS             0x0c
#define STATIC_MEM          0x0e
#define FLAGS_2             0x10
#define ABBRV               0x18
#define FILE_SIZE           0x1a
#define CHECKSUM            0x1c
#define TERP_NUMBER         0x1e
#define TERP_VERSION        0x1f
/* version 4 and above - not yet supported */
#define SCREEN_HEIGHT       0x20
#define SCREEN_WIDTH        0x21
#define SCREEN_HEIGHT_U     0x22
#define SCREEN_WIDTH_U      0x24
#define FONT_WIDTH_U        0x26
#define FONT_HEIGHT_U       0x27
#define ROUTINE_OFFSET      0x28
#define STRINGS_OFFSET      0x2a
#define BACKGROUND_COLOUR   0x2c
#define FOREGROUND_COLOUR   0x2d
#define TERM_CHAR_TAB       0x2e
#define PIXEL_WIDTH         0x30
#define STANDARD_REVISION   0x32
#define ALPHABET_TAB        0x34
#define HEADER_EXT_TAB      0x36

/* We attempt to support version 3 - 8 games (but not 6)*/
#define Z_VERSION_1			0x01
#define Z_VERSION_2			0x02
#define Z_VERSION_3			0x03
#define Z_VERSION_4			0x04
#define Z_VERSION_5			0x05
#define Z_VERSION_6			0x06
#define Z_VERSION_7			0x07
#define Z_VERSION_8			0x08

/* function declarations */
int zerp_run();
void show_status_line();
void fatal_error(char *message);
int glk_printf(char *format, ...);
void set_screen_width(winid_t win);
static void set_header_flags();
static zword_t scan_table(zword_t item, zword_t table, zword_t length, zbyte_t form);

/* The story, upper and status windows. */
extern winid_t mainwin;
extern winid_t statuswin;
extern winid_t upperwin;

/* Some large macros to keep opcode stuff in line in the main loop */
#define get_operand(opnum) (operands[opnum].type == VARIABLE ? variable_get(operands[opnum].bytes) : operands[opnum].bytes)
#define get_operand_ptr(op_ptr) (op_ptr->type == VARIABLE ? variable_get(op_ptr->bytes) : op_ptr->bytes)

#define branch_op(branch_test) if ((branch_test) ^ !(branch_operand.test)) { \
    if (branch_operand.offset == 0 || branch_operand.offset == 1) { \
        return_zroutine(branch_operand.offset); \
    } else { \
        zPC += branch_operand.offset - 2; \
    } \
} 

#define store_op(store_exp) variable_set(store_operand, store_exp);

#define unimplemented(opcode) LOG(ZERROR, "Unimplemented opcode:%s", opcode); fatal_error("UNIMPLEMENTED");
#endif /* ZERP_H */