/* 
    Zerp: a Z-machine interpreter
    objects.h : object functions
*/

#define byte_swap(word) (((word & 0xff) << 8) | ((word & 0xff00) >> 8))

typedef struct zobject_v3 {
    zbyte_t attributes[4];
    zbyte_t parent;
    zbyte_t sibling;
    zbyte_t child;
    zbyte_t properties[2];
} zobject_v3_t;

typedef struct zobject_v4 {
    zbyte_t attributes[6];
    zword_t parent;
    zword_t sibling;
    zword_t child;
    zbyte_t properties[2];
} zobject_v4_t;

#define V3_PROP_SIZE 5
#define V4_PROP_SIZE 5
#define V3_PROP_NUM 0x1f
#define V4_PROP_NUM 0x3f
#define V4_PROP_LEN_MASK 0x80
#define V4_SHORT_PROP_MASK 0x40

/*
zobject_t *get_object(int number);
zword_t object_property_table(int number);
*/
int object_in(int object, int parent);
int object_parent(int object);
int object_sibling(int object);
int object_child(int object);
int remove_object(int object);
void print_object_name(int number);
int get_attribute(int object, int attribute);
int set_attribute(int object, int attribute);
int clear_attribute(int object, int attribute);
zword_t get_property_address(int object, int property);
zword_t get_property_length(zword_t property_address);
zword_t get_property(int object, int property);
zword_t put_property(int object, int property, zword_t value);
int get_next_property(int object, int property);
