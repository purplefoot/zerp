/* 
    Zerp: a Z-machine interpreter
    opcodes.h : debugging monitor
*/

#ifndef DEBUG_H
#define DEBUG_H

void debug_monitor();
static void debug_print_callstack();
static void debug_print_global(int number);
static void debug_print_help();
static void debug_print_local(int number);
static void debug_print_stack();
static void debug_print_memory(int address);
static void debug_print_zstring(packed_addr_t address);
static void dump_stack();
static void dump_globals();
static void dump_locals();

#endif /* DEBUG_H */