/* 
    Zerp: a Z-machine interpreter
    parse.h : parsing and tokenising functions
*/

zword_t read(zword_t input_buffer, zword_t parse_buffer);
void tokenise(zword_t text, zword_t parse_buffer);