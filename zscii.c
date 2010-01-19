/*
    Zerp: a Z-machine interpreter
    zscii.c : z-machine string encoding
*/

#include <stdlib.h>
#include "glk.h"
#include "zerp.h"
#include "zscii.h"

int print_zstring(packed_addr_t address) {
    int words = 0, alpha = 0, zscii = 0, zsciichar = 0, bitshift, abbrv_index;
    zword_t zword;
    unsigned char zchar, abbriv = 0;
    
    int max = 5;
    while(1) {
        zword = get_word(address);
        for (bitshift = 10; bitshift >= 0; bitshift = bitshift - 5) {
            zchar = (unsigned char) (zword >> bitshift) & 0x1f;
            if (zscii) {
                zsciichar = (zsciichar << 5) | zchar;
                zscii++;
                if (zscii++ > 2) {
                    glk_put_char((unsigned char)zsciichar);                    
                    zsciichar = 0; zscii = 0;                            
                }
            } else if (abbriv) {
                abbrv_index = (32 * (abbriv - 1) + zchar);
                print_zstring(get_word(get_word(ABBRV) + (abbrv_index * 2)) * 2);
                abbriv = 0;
            } else {
                switch (zchar) {
                    case 0:
                        alpha = 0;
                        glk_put_char(' ');
                        break;
                    case 1:
                    case 2:
                    case 3:
                        abbriv = zchar;
                        break;
                    case 4:
                    case 5:
                        alpha = zchar - 3;
                        break;
                    case 6:
                        if (alpha == 2) {
                            zscii = 1;
                            break;
                        }
                    default:
                        glk_put_char(zAlphabet[alpha][zchar]);
                        alpha = 0;
                    
                }   
            }
        }
        words++; address += 2;
        if (zword & 0x8000)
            break;
    }
    
    return words * 2;
}
