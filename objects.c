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

int insert_object(int object, int destination) {
    zword_t old_sibling;
    zobject_t *obj, *dest, *obj_parent;

    obj = get_object(object);
    dest = get_object(destination);
    obj_parent = get_object(obj->parent);

    if (obj_parent->child == object)
        obj_parent->child = obj->sibling;

    obj->sibling = dest->child;
    dest->child = object;
    obj->parent = destination;

    return destination;
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

int get_attribute(int object, int attribute) {
    int bit;

    bit = -((attribute % 8) - 7);
    return (get_object(object)->attributes[attribute / 4] >> bit) & 1;
}

int set_attribute(int object, int attribute) {
    int bit;
    zobject_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object(object);
    obj->attributes[attribute /4] = obj->attributes[attribute /4] | bit;
    return 1;
}

int clear_attribute(int object, int attribute) {
    int bit;
    zobject_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object(object);
    obj->attributes[attribute /4] = obj->attributes[attribute /4] & ~bit;
    return 0;
}
