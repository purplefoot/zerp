/* 
    Zerp: a Z-machine interpreter
    objects.c : object functions
*/

#include "glk.h"
#include "zerp.h"
#include "objects.h"

zobject_t *get_object(int number) {
    if (number == 0 || number > 255) {
        fatal_error("Illegal object number");
    }
    return (zobject_t *) (zMachine + zObjects + sizeof(zobject_t) * (number - 1));
}

zword_t object_property_table(int number) {
    zobject_t *o;
    
    o = get_object(number);
    return (zword_t) o->properties[0] << 8 | o->properties[1];
}

zword_t object_properties(int number) {
    zword_t prop_table;

    prop_table = object_property_table(number);
    return prop_table + (get_byte(prop_table) * 2) + 1;
}

zword_t get_property_address(int object, int property) {
    zbyte_t prop_len;
    zword_t prop_ptr;

    prop_ptr = object_properties(object);

    while (prop_len = get_byte(prop_ptr++)) {
        if ((prop_len & V3_PROP_NUM) == property)
            break;
        prop_ptr += (prop_len >> V3_PROP_SIZE) + 1;
    }

    return prop_len ? prop_ptr : prop_len;
}

zword_t get_property_length(zword_t property_address) {
    return (zword_t)(get_byte(property_address -1) >> V3_PROP_SIZE) + 1;
}

zword_t get_property(int object, int property){
    zbyte_t prop_len;
    zword_t prop_ptr;

    if (prop_ptr = get_property_address(object, property)) {
        if ((get_byte(prop_ptr - 1) >> V3_PROP_SIZE) + 1 == 1)
            return get_byte(prop_ptr);
        return get_word(prop_ptr);
    }

    return get_word(zProperties + (property - 1) * 2);
}

zword_t put_property(int object, int property, zword_t value) {
    zbyte_t prop_len;
    zword_t prop_ptr;

    if (prop_ptr = get_property_address(object, property)) {
        if (get_property_length(prop_ptr) == 1) {
            store_byte(prop_ptr, value);
            return value;
        }
        store_word(prop_ptr, value);
        return value;
    }

    fatal_error("Attempted to write a non-existant property");
}

int get_next_property(int object, int property) {
    zbyte_t prop_len;
    zword_t prop_ptr;
    int found = FALSE;

    prop_ptr = object_properties(object);

    while ((prop_len = get_byte(prop_ptr++)) && !found) {
        if (!property)
            return prop_len & V3_PROP_NUM;
         if ((prop_len & V3_PROP_NUM) == property)
            found = TRUE;
        prop_ptr += (prop_len >> V3_PROP_SIZE) + 1;
    }

    return prop_len & V3_PROP_NUM;
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
	zobject_t *obj, *obj_parent;
	zbyte_t prev_sibling;

    obj = get_object(object);
    obj_parent = get_object(obj->parent);

    if (obj_parent->child == object) {
        obj_parent->child = obj->sibling;
    } else {
        for (prev_sibling = obj_parent->child; get_object(prev_sibling)->sibling != object; prev_sibling = get_object(prev_sibling)->sibling) ;
        get_object(prev_sibling)->sibling = obj->sibling;
    }

    obj->parent = 0;

    return 0;
}

int insert_object(int object, int destination) {
    zword_t old_sibling;
    zobject_t *obj, *dest, *obj_parent;

    obj = get_object(object);
    dest = get_object(destination);

    if (obj->parent)
        remove_object(object);

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
    return (get_object(object)->attributes[attribute / 8] >> bit) & 1;
}

int set_attribute(int object, int attribute) {
    int bit;
    zobject_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object(object);
    obj->attributes[attribute /8] = obj->attributes[attribute /4] | bit;
    return 1;
}

int clear_attribute(int object, int attribute) {
    int bit;
    zobject_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object(object);
    obj->attributes[attribute /8] = obj->attributes[attribute /4] & ~bit;
    return 0;
}
