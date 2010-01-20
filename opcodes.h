/* 
    Zerp: a Z-machine interpreter
    opcodes.h : z-machine opcode definitions
*/

#ifndef OPCODES_H
#define OPCODES_H

typedef struct zinstruction {
    zword_t bytes;
    zbyte_t opcode;
    zbyte_t form;
    zbyte_t count;
    zbyte_t store_flag;
    zbyte_t branch_flag;
} zinstruction_t;

typedef struct zoperand {
    zword_t bytes;
    zbyte_t type;
} zoperand_t;

typedef struct zbranch {
    zword_t bytes;
    zbyte_t type;
    zbyte_t test;
    signed short offset;
} zbranch_t;
    
int decode_instruction(packed_addr_t pc, zinstruction_t *instruction, zoperand_t *operands, zword_t *store, zbranch_t *branch);
void print_zinstruction(packed_addr_t instructionPC, zinstruction_t *instruction, zoperand_t *operands,
    zword_t *store_operand, zbranch_t *branch_operand, int flags);
inline static int decode_variable(packed_addr_t *pc, zinstruction_t *instruction, zbyte_t optypes, zoperand_t *operands);
inline static int decode_short(packed_addr_t *pc, zinstruction_t *instruction, zoperand_t *operands);
inline static int decode_long(packed_addr_t *pc, zinstruction_t *instruction, zoperand_t *operands);
static int decode_store_branch(packed_addr_t *pc, zinstruction_t *instruction);
static void decode_branch_op(packed_addr_t *pc, zinstruction_t *instruction, zbranch_t *branch);
static void decode_store_op(packed_addr_t *pc, zinstruction_t *instruction, zword_t *store);
static char * opcode_name(char *buf, zbyte_t opcount, zbyte_t opcode);
static void print_variable(zbyte_t number, int flags);
static void print_operand(zoperand_t *op_ptr);
static void print_operand_list(zoperand_t *op_ptr);

/* Z-machine opcode/operand masks */

#define LARGE_CONST         0x0
#define SMALL_CONST         0x1
#define VARIABLE            0x2
#define NONE                0x3

#define OP_VARIABLE         0x3
#define OP_SHORT            0x2
#define OP_EXTENDED         0xbe
#define OP_LONG             0x1

#define COUNT_0OP           0x0
#define COUNT_1OP           0x1
#define COUNT_2OP           0x2
#define COUNT_VAR           0x3

#define OPCODE_4BIT         0x0f
#define OPCODE_5BIT         0x1f

#define BRANCH_SHORT        0x1
#define BRANCH_LONG         0x2

/* 2OP codes */
#define JE                  0x01
#define JL                  0x02
#define JG                  0x03
#define DEC_CHK             0x04
#define INC_CHK             0x05
#define JIN                 0x06
#define TEST                0x07
#define OR                  0x08
#define AND                 0x09
#define TEST_ATTR           0x0a
#define SET_ATTR            0x0b
#define CLEAR_ATTR          0x0c
#define STORE               0x0d
#define INSERT_OBJ          0x0e
#define LOADW               0x0f
#define LOADB               0x10
#define GET_PROP            0x11
#define GET_PROP_ADDR       0x12
#define GET_NEXT_PROP       0x13
#define ADD                 0x14
#define SUB                 0x15
#define MUL                 0x16
#define DIV                 0x17
#define MOD                 0x18
/* 1OP codes */
#define JZ                  0x00        
#define GET_SIBLING         0x01        
#define GET_CHILD           0x02        
#define GET_PARENT          0x03        
#define GET_PROP_LEN        0x04        
#define INC                 0x05        
#define DEC                 0x06        
#define PRINT_ADDR          0x07        
#define REMOVE_OBJ          0x09        
#define PRINT_OBJ           0x0a        
#define RET                 0x0b        
#define JUMP                0x0c        
#define PRINT_PADDR         0x0d        
#define LOAD                0x0e        
#define NOT                 0x0f        
/* 0OP codes */
#define RTRUE               0x00    
#define RFALSE              0x01    
#define PRINT               0x02    
#define PRINT_RET           0x03    
#define NOP                 0x04    
#define SAVE                0x05    
#define RESTORE             0x06    
#define RESTART             0x07    
#define RET_POPPED          0x08    
#define POP                 0x09    
#define QUIT                0x0a    
#define NEW_LINE            0x0b    
#define SHOW_STATUS         0x0c    
#define VERIFY              0x0d    
/* VAR codes */
#define CALL                0x00     
#define STOREW              0x01     
#define STOREB              0x02     
#define PUT_PROP            0x03     
#define SREAD               0x04     
#define PRINT_CHAR          0x05     
#define PRINT_NUM           0x06
#define RANDOM              0x07
#define PUSH                0x08     
#define PULL                0x09     
#define SPLIT_WINDOW        0x0a     
#define SET_WINDOW          0x0b     
#define OUTPUT_STREAM       0x13     
#define INPUT_STREAM        0x14     
#define SOUND_EFFECT        0x15     

/* printing flags */
#define NO_BYTES            0x01
#define PRINT_READ          0x02
#define PRINT_STORE         0x04

#endif /* OPCODES_H */
