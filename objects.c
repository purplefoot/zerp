/* 
    Zerp: a Z-machine interpreter
    objects.c : object functions
*/

#include "glk.h"
#include "zerp.h"
#include "objects.h"

zobject_t *get_object(int number) {
    if (number == 0 || number > 255)
        fatal_error("Illegal object number");
    return (zobject_t *) (zMachine + zObjects + sizeof(zobject_t) * (number - 1));
}

zword_t object_property_table(int number) {
    zobject_t *o;
    
    o = get_object(number);
    return (zword_t) o->properties[0] << 8 | o->properties[1];
}

int object_in(int object, int parent) {
    return get_object(object)->parent == parent;
}

int object_parent(int object) {
    return get_object(object)->parent;
}

int object_sibling(int object) {
    return get_object(object)->sibling;
}

int object_child(int object) {
    return get_object(object)->child;
}

int remove_object(int object) {
    return get_object(object)->parent = 0;
}
void print_object_name(int number) {
    zword_t prop_table;
    
    if (!number)
        return;
    prop_table = object_property_table(number);
    if (!get_byte(prop_table))
        return;
    print_zstring(prop_table + 1);
}