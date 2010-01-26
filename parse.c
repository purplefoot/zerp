/* 
    Zerp: a Z-machine interpreter
    parse.c : parsing and tokenising functions
*/

#include "glk.h"
#include "zerp.h"
#include "parse.h"

zword_t read(zword_t input_buffer, zword_t parse_buffer) {
	zbyte_t input_len, parse_len;
	zword_t input_ptr;
	int gotline, len;
	char buffer[257];
	char *cx, *cmd;
	event_t ev;
	
	input_len = get_byte(input_buffer);
	parse_len = get_byte(parse_buffer);
	
	glk_request_line_event(mainwin, buffer, input_len, 0);
    gotline = FALSE;
    while (!gotline) {
        glk_select(&ev);
        if (ev.type == evtype_LineInput)
            gotline = TRUE;
    }

    len = ev.val1;

    for (cx = buffer; *cx; cx++) { 
        *cx = glk_char_to_lower(*cx);
    }
    
    /* strip whitespace */
    for (cx = buffer; *cx == ' '; cx++) { };
    cmd = cx;
    for (cx = buffer + len - 1; cx >= cmd && *cx == ' '; cx--) { };
    *(cx+1) = '\0';

	input_ptr = input_buffer + 1; len = 0;
	while (*cmd != '\0') {
		store_byte(input_ptr++, *cmd++);
		len++;		
	}
	
	store_byte(input_buffer, (zbyte_t) len);
	
	tokenise(input_buffer, parse_buffer);
}

void tokenise(zword_t text, zword_t parse_buffer) {
	
}
