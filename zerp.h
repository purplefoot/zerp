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

#define SMALLBUFF 1024

/* Z-machine address formats */
typedef unsigned short zByteAddr;
#define game_byte(offset) *(zMachine + offset)
#define store_game_byte(offset, value) game_byte(offset) = (unsigned char) value;
#define game_loc(offset) (zMachine + offset)
#define byte_addr(addr) (((zByteAddr) game_byte(addr)) << 8 | game_byte(addr + 1))
#define store_byte_addr(offset, value) store_game_byte(offset, value >> 8); store_game_byte(offset + 1,value & 0xff);
#define word_addr(addr) byte_addr(addr) >> 1
#define packed_addr(addr) byte_addr(addr) << 1
#define unpack(addr) addr << 1

/* game file */
extern char * zFilename;
extern int zFilesize;
extern unsigned char * zGamefile;
extern unsigned char * zMachine;

/* z-machine stack */
extern zByteAddr * zStack;
extern zByteAddr * zSP;
extern zByteAddr * zStackTop;
extern zByteAddr * zFP;
extern zByteAddr zGlobals;
#define STACKSIZE 8192
extern unsigned short zPC;

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

/* We only like version 3 games */
#define Z_VERSION_3         0x03

/* function declarations */
int zerp_run();
int glk_printf(char *format, ...);

#endif /* ZERP_H */