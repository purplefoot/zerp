/* 
    Zerp: a Z-machine interpreter
    debug.c : debugging monitor
*/

#include <regex.h>
#include <string.h>
#ifdef TARGET_OS_MAC
#include <GlkClient/glk.h>
#else
#include "glk.h"
#endif /* TARGET_OS_MAC */
#include "zerp.h"
#include "objects.h"
#include "debug.h"

#define match_command(command) strncmp(command, cmd + pmatch[1].rm_so, 1) == 0

#define match_args_and_call(func, failed_match) if (pmatch[3].rm_so != pmatch[3].rm_eo) { \
        func(strtol(cmd + pmatch[3].rm_so, NULL, 0)); \
    } else { \
        func(failed_match); \
    }

/* event parsing loop freely stolen from Zarf's glk example code */
void debug_monitor() {
    char commandbuf[256], lastcommand[256];
    char *cx, *cmd;
    int gotline, len, monitor, match;
    event_t ev;
    char *parser = "^([cfghlnoqsx])( +([0-9a-f]+))?";
    // char *parser = "\\([cghlnqsx]\\)\\( 0\\)?";
    char *matched;
    regex_t preg;
    size_t nmatch = 4;
    regmatch_t pmatch[4];
    
    monitor = TRUE;
    while(monitor) {
        glk_put_string("\nmonitor>");
        // if (cmd)
        //     strncpy(lastcommand, cmd, 256);
        glk_request_line_event(mainwin, commandbuf, 255, 0);
        gotline = FALSE;
        while (!gotline) {
            glk_select(&ev);
            if (ev.type == evtype_LineInput)
                gotline = TRUE;
        }

        len = ev.val1;
        commandbuf[len] = '\0';

        for (cx = commandbuf; *cx; cx++) { 
            *cx = glk_char_to_lower(*cx);
        }
        
        /* strip whitespace */
        for (cx = commandbuf; *cx == ' '; cx++) { };
        cmd = cx;
        for (cx = commandbuf+len-1; cx >= cmd && *cx == ' '; cx--) { };
        *(cx+1) = '\0';
        
        if (*cmd == '\0') {
            monitor = FALSE;
            continue;
        }
        
        if ((match = regcomp(&preg, parser, REG_EXTENDED | REG_ICASE)) != 0) {
            fatal_error("Bad regex\n");
        }

        if ((match = regexec(&preg, cmd, nmatch, pmatch, 0)) != 0) {
            glk_put_string("pardon? - try 'h' for help\n");
        } else {
            if(match_command("c")) {
                monitor = FALSE;
            } else if (match_command("f")) {
                match_args_and_call(debug_print_callstack, 0xffff)
            } else if (match_command("g")) {
                match_args_and_call(debug_print_global, 0xff)
            } else if (match_command("h")) {
                debug_print_help();
            } else if (match_command("l")) {
                match_args_and_call(debug_print_local, 0xff)
            } else if (match_command("n")) {
                monitor = FALSE;                
            } else if (match_command("o")) {
                match_args_and_call(debug_print_object, 1);         
            } else if (match_command("x")) {
                match_args_and_call(debug_print_memory, 0xdeadbeef)
            } else if (match_command("")) {
                match_args_and_call(debug_print_zstring, 0)
            } else if (match_command("q")) {
                fatal_error("Debugger terminated game.");
            } else if (match_command("s")) {
                debug_print_stack();
            } else if (match_command("x")) {
                match_args_and_call(debug_print_memory, 0xdeadbeef)
            }
        }
        regfree(&preg);
    }
}

static void debug_print_callstack(int frame) {
    int i, count;
    zstack_frame_t *aframe;
    zword_t *stack_top, *stack_bottom;
    
    if (frame == 0xffff) {
        i = zFP - zCallStack;
        while (i >= 0) {
            aframe = zCallStack + i;
            glk_printf("%2d (%08x): pc:%05x sp:%08x throw_ret:%s, args: %d ret:",
						i, aframe, aframe->pc, aframe->sp, (aframe->ret_keep ? "throw" : "keep"), aframe->args);
            if (aframe->ret_store == 0) {
                glk_put_string("SP");
            } else if (aframe->ret_store > 0 && aframe->ret_store < 0x10) {
                glk_printf("L%02x", aframe->ret_store - 1);
            } else {
                glk_printf("G%02x", aframe->ret_store - 0x10);
            }
            glk_put_string("\n"); i--;
        }
    } else {
        if (frame < 0 || frame > zFP - zCallStack) {
            glk_put_string("Invalid call frame.\n");
            return;
        }
        aframe = zCallStack + frame;
        glk_printf("Call frame %2d (%08x):\n pc:%05x\n throw_ret:%s\n args: %d\n sp:%08x - ",
						frame, aframe, aframe->pc, (aframe->ret_keep ? "throw" : "keep"), aframe->args, aframe->sp);
        if (frame > 0) {
            stack_top = aframe->sp; stack_bottom = (zCallStack + (frame -1))->sp;
            count = 0;
            while (stack_top > stack_bottom && ++count < 6)
                glk_printf(" %04x", *--stack_top);
        }
        
        glk_put_string("\n return value in:");
        if (aframe->ret_store == 0) {
            glk_put_string("SP");
        } else if (aframe->ret_store > 0 && aframe->ret_store < 0x10) {
            glk_printf("L%02x", aframe->ret_store - 1);
        } else {
            glk_printf("G%02x", aframe->ret_store - 0x10);
        }
        glk_put_string("\nLocals:\n");
        
        for (i = 1; i < 16; i++) {
            glk_printf("%8x", aframe->locals[i]);
        }
    }
}

static void debug_print_global(int number) {
    if (number == 0xff) {
        dump_globals();
    } else {
        if (number > 255) {
            glk_put_string("ERROR: global out of range\n");
            return;
        }
        if (number == 0) {
            glk_printf("G%02x : %04x\n", number, *zSP);
            return;
        }
        glk_printf("G%02x : %04x\n", number, variable_get(number));
    }
}

static void debug_print_help() {
    glk_put_string("zerp internal debugger\n\n");
    glk_put_string("c               - continue execution\n");
    glk_put_string("f [frameno]     - print call stack frame, omit arg to show all frames\n");
    glk_put_string("g [globalno]    - print global variable <globalno>, omit arg to see all globals\n");
    glk_put_string("h               - this help information\n");
    glk_put_string("l [localno]     - print local variable <localno>, omit arg to see all locals\n");
    glk_put_string("n               - execute next instruction\n");
    glk_put_string("p [address]     - print zstring at <address>\n");
    glk_put_string("q               - quit game\n");
    glk_put_string("s               - print stack contents\n");
    glk_put_string("x [address]     - examine memory at <address>\n");
    glk_put_string("\n");
}

static void debug_print_local(int number){
    if (number == 0xff) {
        dump_locals();
    } else {
        if (number < 1 || number > 15) {
            glk_put_string("ERROR: local out of range\n");
            return;
        }
        glk_printf("L%02x : %04x\n", number, variable_get(number));
    }
}

static void debug_print_stack() {
    dump_stack();
}

static void debug_print_memory(int address) {
    int i;
    
    if (address < 0 || address > zFilesize) {
        glk_printf("ERROR: address out of range (0 - %08x)\n", zFilesize);
        return;
    }
    glk_printf("%#08x : ", address);
    for (i = 0; i < 16 ; i+=2)
        glk_printf("  %04x", get_word(address + i));
    glk_put_string("\n");
}

static void debug_print_zstring(packed_addr_t address) {
    print_zstring(address);
}

static void debug_print_object(int number) {
	int i;
	zword_t addr, len;
	
    glk_printf("Object %d:\nName: \"", number);
    print_object_name(number);
    glk_put_string("\"\nAttributes: ");
	for (i = 0; i < 32; i++) {
		if (get_attribute(number, i))
			glk_printf("%d ", i);
	} 
    glk_printf("\nParent: %3d \"", object_parent(number));
    print_object_name(object_parent(number));
    glk_printf("\"\nSibling: %3d \"", object_sibling(number));
    print_object_name(object_sibling(number));
    glk_printf("\"\nChild: %3d \"", object_child(number));
    print_object_name(object_child(number));
    glk_put_string("\"\nProperties:\n");
	i = 0;
	while (i = get_next_property(number, i)) {
		glk_printf("    [%2d] ", i);
		addr = get_property_address(number, i);
		for (len = get_property_length(addr); len > 0; len--)
			glk_printf("%02x ", get_byte(addr++));
		glk_put_string("\n");
	}
}

static void dump_stack() {
    zword_t *s;
    int empty = TRUE;
    
    s = zSP;
    while(s > zFP->sp) {
        glk_printf("%#08x : %04x\n", s, *s--);
        empty = FALSE;
    }
    if (empty)
        glk_put_string("Stack empty.");
    glk_put_string("\n");
}

static void dump_globals() {
    int g, x, y;

    glk_printf("\nglobal variables starting at %#08x:\n", zGlobals);    
    glk_put_string("  ");
    for (x = 0; x < 16; x++)
        glk_printf("%8x", x);
    glk_put_string("\n");

    for (g = 0; g < 240; g++) {
        if (!(g % 16))
            glk_printf("\n%02x", g);
        glk_printf("%8x", get_word(zGlobals + ((g) * 2)));
    }
    glk_put_string("\n");
}

static void dump_locals() {
    int l, x, y;
    
    glk_printf("\nlocal variables in stack frame %#08x:\n", zFP);
    for (x = 1; x < 16; x++)
        glk_printf("%8x", x);
    glk_put_string("\n");

    for (l = 1; l < 16; l++) {
        // if (l == 1 || l == 9)
        //     glk_printf("\n%02x", l);
        glk_printf("%8x", variable_get(l));
    }
    glk_put_string("\n");
}
