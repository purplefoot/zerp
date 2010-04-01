/* 
    Zerp: a Z-machine interpreter
    parse.c : parsing and tokenising functions
*/

#include <stdio.h>
#ifdef TARGET_OS_MAC
#include <GlkClient/glk.h>
#else
#include "glk.h"
#endif /* TARGET_OS_MAC */
#include "zerp.h"
#include "zscii.h"
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

	if (zGameVersion < Z_VERSION_5) {
		input_ptr = input_buffer + 1;
		len = 0;
	} else {
		if (len = get_byte(input_buffer + 1)) {
			input_ptr = input_buffer + 2 + get_byte(input_buffer + 1);	
		} else {
			input_ptr = input_buffer + 2;				
		}
	}
	
	while (*cmd != '\0') {
		store_byte(input_ptr++, *cmd++);
		len++;		
	}
	
	if (zGameVersion < Z_VERSION_5) {
		store_byte(input_ptr, '\0');		
	} else {
		store_byte(input_buffer + 1, len);
	}
	
	if (zGameVersion < Z_VERSION_5 || parse_buffer)
		tokenise(input_buffer, parse_buffer, 0, 0);
	
	return 0xa;
}

zbyte_t read_char(zword_t device) {
	int gotchar;
	event_t ev;


	glk_request_char_event(mainwin);
    gotchar = FALSE;
    while (!gotchar) {
        glk_select(&ev);
        if (ev.type == evtype_CharInput)
            gotchar = TRUE;
    }

	return ev.val1;
}

void tokenise(zword_t text, zword_t parse_buffer, zword_t dictionary, zword_t flag) {
	zword_t token_start, current, input_size, buf_start, word_seps, max_entries, dict_address, pbufftmp;
	zbyte_t max_tokens, total_seps, tokens_total;
	char token[257];
	zword_t zstring[3];
	int char_count, done, token_found, sep_found;
	
	pbufftmp = parse_buffer;
	max_tokens = get_byte(parse_buffer++);
	parse_buffer++; /* space for total tokens found */
	if (zGameVersion > Z_VERSION_4)
		input_size =  get_byte(++text);
	total_seps = get_byte(zDictionaryHeader);
	word_seps = zDictionaryHeader + 1;

	done = FALSE; token_found = FALSE; sep_found = FALSE;
	tokens_total = 0;
	token_start = current = ++text;
	while (token_found || get_byte(current) != '\0' || tokens_total >= max_tokens || (zGameVersion > Z_VERSION_4 && input_size-- > 0)) {
		sep_found = check_separator(word_seps, total_seps, get_byte(current));
		if (token_found) {
			if (get_byte(current) == ' ' || get_byte(current) == '\0' || sep_found ) {
				char_count = 0;
				buf_start = token_start;
				while (buf_start < current)
					token[char_count++] = get_byte(buf_start++);
				encode_zstring(token, char_count, zstring, (zGameVersion < Z_VERSION_4 ? DICT_RESOLUTION_V3 : DICT_RESOLUTION_V4));
				dict_address = lookup_entry(zstring);
				store_word(parse_buffer, dict_address);
				parse_buffer += 2;
				store_byte(parse_buffer++, char_count);
				store_byte(parse_buffer++, token_start - text + 1);
				tokens_total++;
				if (get_byte(current) == '\0')
					break;
				if (sep_found) {
					token[0] = get_byte(current++);
					encode_zstring(token, 1, zstring, (zGameVersion < Z_VERSION_4 ? DICT_RESOLUTION_V3 : DICT_RESOLUTION_V4));
					dict_address = lookup_entry(zstring);
					store_word(parse_buffer, dict_address);
					parse_buffer += 2;
					store_byte(parse_buffer++, 1);
					store_byte(parse_buffer++, current - text);
					tokens_total++;
				}
				token_found = FALSE;
				current++;
				continue;
			}
			current++;
			continue;
		}
		if (get_byte(current) == ' ') {
			current++;
			continue;
		}
		if (sep_found) {
			token[0] = get_byte(current++);
			encode_zstring(token, 1, zstring, (zGameVersion < Z_VERSION_4 ? DICT_RESOLUTION_V3 : DICT_RESOLUTION_V4));
			dict_address = lookup_entry(zstring);
			store_word(parse_buffer, dict_address);
			parse_buffer += 2;
			store_byte(parse_buffer++, 1);
			store_byte(parse_buffer++, current - text);
			tokens_total++;
			continue;
		}
		token_found = TRUE;
		token_start = current++;
		continue;
	}
	LOG(ZDEBUG, "\nParse table: (%d words)", tokens_total);
	for (char_count = 0; char_count < tokens_total;char_count++) {
		LOG(ZDEBUG, "\ndictionary word: %04x start: %02x  length: %02x",
		 			get_word(pbufftmp + (char_count *4) + 2),
					get_byte(pbufftmp + (char_count *4) + 5),
					get_byte(pbufftmp + (char_count *4) + 4));
	}

	store_byte(pbufftmp + 1, tokens_total);
	return;
}

int check_separator(zword_t separators, zbyte_t total, zbyte_t value) {
	zword_t last_sep;

	last_sep = separators + total;
	while (separators < last_sep) {
		if (get_byte(separators++) == value)
			return TRUE;
	}
	return FALSE;
}

void encode_zstring(char *token_buffer, int buf_len, zword_t *zstring, int zstring_len) {
	int zword, i;
	zbyte_t zchar, shifted;
	zword_t encoded_word;
	char *token_end;

	token_end = token_buffer + buf_len;
	shifted = 0;
	while (zstring_len-- > 0) {
		zword = 0;
		encoded_word = 0;
		while (zword++ < 3) {
			if (shifted || (token_buffer < token_end)) {
				if (shifted) {
					zchar = shifted;
					shifted = 0;
				} else if (*token_buffer >= 'a' && *token_buffer <= 'z') {
					/* turn a-z in a0 alphabet. FIXME: this won't fly for custom alphabets in v5 obviously */
					zchar = *(token_buffer++) - 91; /* lowercase ascii to a0 */
				} else {
					/* FIXME: start zscii sequence */

					/* FIXME: lookup in a2 */
					for (i = 7; i < 32; i++) {
						if (zAlphabet[2][i] == *token_buffer) {
							shifted = i;
							zchar = 5;
							token_buffer++;
							break;
						}
					}
				}				
			} else {
				/* finished word, add padding zchars */
				zchar = 5;
			}
			encoded_word = (encoded_word << 5) | zchar;
		}
		if (zstring_len == 0)
			encoded_word |= 0x8000; /* mark end of zstring */
		*zstring++ = encoded_word;
	}
}

zword_t lookup_entry(zword_t *zstring) {
	zword_t word, total_entries;
	zbyte_t entry_length;

	entry_length = get_byte(zDictionaryHeader + get_byte(zDictionaryHeader) + 1);
	total_entries = get_word(zDictionaryHeader + get_byte(zDictionaryHeader) + 2);
	/* Do a linear search - faster algo to come */
	for (word = 0; word < total_entries; word++) {
		if (get_word(zDictionary + (word * entry_length)) == *zstring) {
			if (get_word(zDictionary + (word * entry_length) + 2) == *(zstring + 1))
				return zDictionary + (word * entry_length);
		}
	}

	return 0;
}