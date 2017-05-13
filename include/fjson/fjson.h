#ifndef FJSON_H
#define FJSON_H

#include <stdlib.h>

// TYPES
typedef enum {
    FJSON_TYPE_UNKNOWN,
    FJSON_TYPE_OBJECT,
    FJSON_TYPE_ARRAY,
    FJSON_TYPE_STRING,
    FJSON_TYPE_NUMBER,
    FJSON_TYPE_BOOLEAN,
    FJSON_TYPE_NULL
} fjson_type_t;


struct fjson_pair_s;
struct fjson_array_s;

typedef struct fjson_element_s {
    fjson_type_t type;
    union {
        char *str; // type == FJSON_TYPE_STRING
        double num; // type == FJSON_TYPE_NUMBER
        struct fjson_array_s *array; // type == FJSON_TYPE_ARRAY
        struct fjson_pair_s *pairs; // type == FJSON_TYPE_OBJECT
        unsigned char bool_val; // type == FJSON_TYPE_BOOLEAN
    };
} fjson_element_t;

/*
{
    "key": "string", <--- PAIR
    "key2": 12345, <--- PAIR
    "key3": {...}, <--- PAIR
    "key4": [...], <--- PAIR
}
*/

typedef struct fjson_pair_s {
    fjson_element_t *key;
    fjson_element_t *value;
    struct fjson_pair_s *next;
} fjson_pair_t;

typedef struct fjson_array_s {
    fjson_element_t *el;
    struct fjson_array_s *next;
} fjson_array_t;

typedef struct fjson_s {

    int state;
    fjson_element_t *el;
    char *buf;
    size_t bi;

    struct fjson_s *father;
    struct fjson_s *child;

} fjson_t;

fjson_t* fjson_new();
void fjson_free(fjson_t *fjson);
void fjson_free_element(fjson_element_t *el);

// Given an element with type FJSON_TYPE_OBJECT and a key it returns the value (if present).
fjson_element_t* fjson_get_value_by_key(fjson_element_t *obj, const char *key);

// Return -1 on error, 0 on no error, 1 on completed parsing element.
// On error all the fjson_putbyte call chain returns error.
int fjson_putbyte(fjson_t *fjson, char byte);
int fjson_putbuf(fjson_t *fjson, char *buf, size_t len);

#endif
