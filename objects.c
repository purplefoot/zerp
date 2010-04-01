/* 
    Zerp: a Z-machine interpreter
    objects.c : object functions
*/

#include <stdio.h>
#ifdef TARGET_OS_MAC
#include <GlkClient/glk.h>
#else
#include "glk.h"
#endif /* TARGET_OS_MAC */
#include "zerp.h"
#include "objects.h"

zobject_v3_t *get_object_v3(int number) {
    if (number == 0 || number > 0xff) {
        fatal_error("Illegal object number 0 or > 255");
    }
    return (zobject_v3_t *) (zMachine + zObjects + sizeof(zobject_v3_t) * (number - 1));
}

zobject_v4_t *get_object_v4(int number) {
    if (number == 0 || number > 0xffff) {
		LOG(ZERROR, "Illegal object requested %i\n", number);
        fatal_error("Illegal object number 0 or > 65534");
    }
    return (zobject_v4_t *) (zMachine + zObjects + sizeof(zobject_v4_t) * (number - 1));
}

zword_t object_property_table_v3(int number) {
    zobject_v3_t *o;

    o = get_object_v3(number);
     return (zword_t) o->properties[0] << 8 | o->properties[1];
}

zword_t object_property_table_v4(int number) {
    zobject_v4_t *o;
    
    o = get_object_v4(number);
	return (zword_t) o->properties[0] << 8 | o->properties[1];
}

zword_t object_properties_v3(int number) {
    zword_t prop_table;

    prop_table = object_property_table_v3(number);
    return prop_table + (get_byte(prop_table) * 2) + 1;
}

zword_t object_properties_v4(int number) {
    zword_t prop_table;

    prop_table = object_property_table_v4(number);
    return prop_table + (get_byte(prop_table) * 2) + 1;
}

zword_t get_property_address_v3(int object, int property) {
    zbyte_t prop_len;
    zword_t prop_ptr;

    prop_ptr = object_properties_v3(object);

    while (prop_len = get_byte(prop_ptr++)) {
        if ((prop_len & V3_PROP_NUM) == property)
            break;
        prop_ptr += (prop_len >> V3_PROP_SIZE) + 1;
    }

    return prop_len ? prop_ptr : 0;
}

zword_t get_property_address_v4(int object, int property) {
    zbyte_t prop_len, prop_num;
    zword_t prop_ptr;

    prop_ptr = object_properties_v4(object);

    while (prop_len = get_byte(prop_ptr++) ) {
		prop_num = prop_len & V4_PROP_NUM;
		if (prop_len & V4_PROP_LEN_MASK) {
			/* 2 byte property size */
			prop_len = (get_byte(prop_ptr++)) & V4_PROP_NUM;
			if (!prop_len)
				prop_len = 64; /* Inform requires this */
		} else {
			/* just one size byte */
			prop_len = ((prop_len & V4_SHORT_PROP_MASK) >> 6) + 1;
		}
        if (prop_num == property)
            break;
        prop_ptr += prop_len;
    }

    return prop_len ? prop_ptr : 0;
}

zword_t get_property_address(int object, int property) {
	if (zGameVersion < Z_VERSION_4) {
		return get_property_address_v3(object, property);
	} else {
		return get_property_address_v4(object, property);
	}
}

zword_t get_property_length(zword_t property_address) {
	zbyte_t size;

	size = get_byte(property_address - 1);
	if (zGameVersion < Z_VERSION_4) {
		return (zword_t)(size >> V3_PROP_SIZE) + 1;
	} else {
		if (size & V4_PROP_LEN_MASK) {
			/* bit 7 set, so this is the size byte of a 2 byte property size. Size of 0 returns 64 for Inform */
			return (zword_t) ((size & V4_PROP_NUM) == 0 ? 64 : size & V4_PROP_NUM);
		} else {
			/* Just 1 byte of size info so bit 6 gives size of 1 or 2 */
			return (zword_t) ((size & V4_SHORT_PROP_MASK) >> 6) + 1;
		}
	}
}

zword_t get_property(int object, int property){
    zbyte_t prop_len;
    zword_t prop_ptr;

	if (zGameVersion < Z_VERSION_4) {
		prop_ptr = get_property_address_v3(object, property);
	} else {
		prop_ptr = get_property_address_v4(object, property);
	}
    if (prop_ptr) {
        if (get_property_length(prop_ptr) == 1)
            return get_byte(prop_ptr);
        return get_word(prop_ptr);
    }

    return get_word(zProperties + (property - 1) * 2);
}

zword_t put_property(int object, int property, zword_t value) {
    zbyte_t prop_len;
    zword_t prop_ptr;

	if (zGameVersion < Z_VERSION_4) {
		prop_ptr = get_property_address_v3(object, property);
	} else {
		prop_ptr = get_property_address_v4(object, property);
	}
    if (prop_ptr) {
        if (get_property_length(prop_ptr) == 1) {
            store_byte(prop_ptr, value);
            return value;
        }
        store_word(prop_ptr, value);
        return value;
    }

    fatal_error("Attempted to write a non-existant property");
}

int get_next_property_v3(int object, int property) {
    zbyte_t prop_len;
    zword_t prop_ptr;
    int found = FALSE;

    prop_ptr = object_properties_v3(object);

    while ((prop_len = get_byte(prop_ptr++)) && !found) {
        if (!property)
            return prop_len & V3_PROP_NUM;
         if ((prop_len & V3_PROP_NUM) == property)
            found = TRUE;
        prop_ptr += (prop_len >> V3_PROP_SIZE) + 1;
    }

    return prop_len & V3_PROP_NUM;
}

int get_next_property_v4(int object, int property) {
	zbyte_t prop_len, prop_num;
    zword_t prop_ptr;
	int found = FALSE;

    prop_ptr = object_properties_v4(object);

    while ((prop_len = get_byte(prop_ptr++)) && !found) {
		prop_num = prop_len & V4_PROP_NUM;
		if (!property)
			return prop_num;
		if (prop_len & V4_PROP_LEN_MASK) {
			/* 2 byte property size */
			prop_len = (get_byte(prop_ptr++)) & V4_PROP_NUM;
			if (!prop_len)
				prop_len = 64; /* Inform requires this */
		} else {
			/* just one size byte */
			prop_len = ((prop_len & V4_SHORT_PROP_MASK) >> 6) + 1;
		}
        if (prop_num == property)
            found = TRUE;
        prop_ptr += prop_len;
    }

	return prop_len & V4_PROP_NUM;
}

int get_next_property(int object, int property) {
	if (zGameVersion < Z_VERSION_4) {
	    return get_next_property_v3(object, property);
	} else {
	    return get_next_property_v4(object, property);
	}
}

int object_in(int object, int parent) {
	if (zGameVersion < Z_VERSION_4) {
	    return get_object_v3(object)->parent == parent;
	} else {
	    return get_object_number_v4(get_object_v4(object), parent) == parent;
	}
}

int object_parent(int object) {
	if (zGameVersion < Z_VERSION_4) {
	    return get_object_v3(object)->parent;
	} else {
	    return get_object_number_v4(get_object_v4(object), parent);
	}
}

int object_sibling(int object) {
	if (zGameVersion < Z_VERSION_4) {
	    return get_object_v3(object)->sibling;
	} else {
	    return get_object_number_v4(get_object_v4(object), sibling);
	}
}

int object_child(int object) {
	if (zGameVersion < Z_VERSION_4) {
	    return get_object_v3(object)->child;
	} else {
	    return get_object_number_v4(get_object_v4(object), child);
	}
}

int remove_object_v3(int object) {
	zobject_v3_t *obj, *obj_parent;
	zbyte_t prev_sibling;

    obj = get_object_v3(object);
    obj_parent = get_object_v3(obj->parent);

    if (obj_parent->child == object) {
        obj_parent->child = obj->sibling;
    } else {
        for (prev_sibling = obj_parent->child; get_object_v3(prev_sibling)->sibling != object; prev_sibling = get_object_v3(prev_sibling)->sibling) ;
        get_object_v3(prev_sibling)->sibling = obj->sibling;
    }

    obj->parent = 0;
	obj->sibling = 0;

    return 0;
}

int remove_object_v4(int object) {
	zobject_v4_t *obj, *obj_parent;
	zword_t prev_sibling;

    obj = get_object_v4(object);
    obj_parent = get_object_v4(get_object_number_v4(obj, parent));

    if (get_object_number_v4(obj_parent, child) == object) {
        set_object_number_v4(obj_parent, child, get_object_number_v4(obj, sibling));
    } else {
        for (prev_sibling = get_object_number_v4(obj_parent, child);
 			 get_object_number_v4(get_object_v4(prev_sibling), sibling) != object;
 			 prev_sibling = get_object_number_v4(get_object_v4(prev_sibling), sibling)) ;
        set_object_number_v4(get_object_v4(prev_sibling), sibling, get_object_number_v4(obj, sibling));
    }

    set_object_number_v4(obj, parent, 0);
	set_object_number_v4(obj, sibling, 0);

    return 0;
}

int remove_object(int object) {
	if (zGameVersion < Z_VERSION_4) {
		remove_object_v3(object);
	} else {
		remove_object_v4(object);
	}
}

int insert_object_v3(int object, int destination) {
    zobject_v3_t *obj, *dest, *obj_parent;

    obj = get_object_v3(object);
    dest = get_object_v3(destination);

    if (obj->parent)
        remove_object_v3(object);

    obj->sibling = dest->child;
    dest->child = object;
    obj->parent = destination;

    return destination;
}

int insert_object_v4(int object, int destination) {
    zobject_v4_t *obj, *dest, *obj_parent;

    obj = get_object_v4(object);
    dest = get_object_v4(destination);

    if (get_object_number_v4(obj, parent) != 0)
        remove_object_v4(object);

    set_object_number_v4(obj, sibling, get_object_number_v4(dest, child));
    set_object_number_v4(dest, child, object);
    set_object_number_v4(obj, parent, destination);

    return destination;
}

int insert_object(int object, int destination) {
	if (zGameVersion < Z_VERSION_4) {
		insert_object_v3(object, destination);
	} else {
		insert_object_v4(object, destination);
	}
}

void print_object_name(int number) {
    zword_t prop_table;
    
    if (!number)
        return;
	if (zGameVersion < Z_VERSION_4) {
	    prop_table = object_property_table_v3(number);
	} else {
	    prop_table = object_property_table_v4(number);
	}
    if (!get_byte(prop_table))
        return;
    print_zstring(prop_table + 1);
}

int get_attribute(int object, int attribute) {
    int bit;

    bit = -((attribute % 8) - 7);
	if (zGameVersion < Z_VERSION_4) {
	    return (get_object_v3(object)->attributes[attribute / 8] >> bit) & 1;
	} else {
	    return (get_object_v4(object)->attributes[attribute / 8] >> bit) & 1;
	}
}

int set_attribute_v3(int object, int attribute) {
    int bit;
    zobject_v3_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object_v3(object);
    obj->attributes[attribute /8] = obj->attributes[attribute / 8] | bit;
    return 1;
}

int set_attribute_v4(int object, int attribute) {
    int bit;
    zobject_v4_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object_v4(object);
    obj->attributes[attribute /8] = obj->attributes[attribute / 8] | bit;
    return 1;
}

int set_attribute(int object, int attribute) {
	if (zGameVersion < Z_VERSION_4) {
		return set_attribute_v3(object, attribute);
	} else {
		return set_attribute_v4(object, attribute);
	}
}

int clear_attribute_v3(int object, int attribute) {
    int bit;
    zobject_v3_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object_v3(object);
    obj->attributes[attribute /8] = obj->attributes[attribute / 8] & ~bit;
    return 0;
}

int clear_attribute_v4(int object, int attribute) {
    int bit;
    zobject_v4_t *obj;

    bit = 1 << -((attribute % 8) - 7);
    obj = get_object_v4(object);
    obj->attributes[attribute /8] = obj->attributes[attribute / 8] & ~bit;
    return 0;
}

int clear_attribute(int object, int attribute) {
	if (zGameVersion < Z_VERSION_4) {
		return clear_attribute_v3(object, attribute);
	} else {
		return clear_attribute_v4(object, attribute);
	}
}
