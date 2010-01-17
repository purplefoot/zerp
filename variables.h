/* 
    Zerp: a Z-machine interpreter
    variables.h : z-machine variable access
*/

#ifndef VARIABLES_H
#define VARIABLES_H

zByteAddr variable_get(unsigned char variable);
int variable_set(unsigned char variable, zByteAddr value);

#endif /* VARIABLES_H */