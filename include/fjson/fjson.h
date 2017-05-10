#ifndef FJSON_H
#define FJSON_H

#include <stdlib.h>


// STATES
typedef enum {
    FJSON_STATE_ELEMENT,
    FJSON_STATE_OBJECT_PAIR,
    FJSON_STATE_OBJECT_KEY,
    FJSON_STATE_OBJECT_KEY_PARSED,
    FJSON_STATE_OBJECT_VALUE,
    FJSON_STATE_OBJECT_AFTER_VALUE,
    FJSON_STATE_ARRAY_VALUE,
    FJSON_STATE_ARRAY_AFTER_VALUE,
    FJSON_STATE_STRING,
    FJSON_STATE_SPEC_CHAR,
    FJSON_STATE_NUMBER,
    FJSON_STATE_BOOLEAN,
    FJSON_STATE_NULL
} fjson_state_t;

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
        char *str;
        int64_t num;
        struct fjson_array_s *array;
        struct fjson_pair_s *pairs;
        unsigned char bool_val;
        void *content;
    };
} fjson_element_t;

// Object key: value
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

    fjson_state_t state;
    fjson_element_t *el;
    char *buf;
    size_t bi;
    
    struct fjson_s *father;
    struct fjson_s *child;

} fjson_t;

fjson_t* fjson_new();
void fjson_free(fjson_t *fjson);
void fjson_free_element(fjson_element_t *el);

// Return -1 on error, 0 on no error, 1 on completed parsing element.
// On error all the fjson_putbyte call chain returns error.

int fjson_putbyte(fjson_t *fjson, char byte);
int fjson_putbuf(fjson_t *fjson, char *buf, size_t len);

#endif
