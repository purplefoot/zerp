/* 
    Zerp: a Z-machine interpreter
    variables.h : z-machine variable access
*/

#ifndef VARIABLES_H
#define VARIABLES_H

zword_t variable_get(unsigned char variable);
int variable_set(unsigned char variable, zword_t value);
int indirect_variable_get(zbyte_t variable, zword_t value);
int indirect_variable_set(zbyte_t variable, zword_t value);

#endif /* VARIABLES_H */