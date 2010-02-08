/* 
    Zerp: a Z-machine interpreter
    parse.h : parsing and tokenising functions
*/

#define DICT_RESOLUTION_V3		2
#define DICT_RESOLUTION_V4		3

zword_t read(zword_t input_buffer, zword_t parse_buffer);
int tokenise(zword_t text, zword_t parse_buffer);
int check_separator(zword_t separators, zbyte_t total, zbyte_t value);
void encode_zstring(char *token_buffer, int buf_len, zword_t *zstring, int zstring_len);
zword_t lookup_entry(zword_t *zstring);