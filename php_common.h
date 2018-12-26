#pragma once
#include <stdint.h>

#define _PHP_MAX_TYPES 8

#define _PHP_FL_BINARY  1
#define _PHP_FL_PRINTED 2
#define _PHP_FL_PRIVATE 4
#define _PHP_FL_PROTECTED 8
#define _PHP_FL_MALLOCED 16

enum var_type {
    Integer = 0,
    Float,
    Boolean,
    Null,
    Reference,
    Object,
    String,
    Array,
    Unknown
};

struct php_element_s {
    enum var_type type;
    uint32_t length;
    uint32_t refs;
    uint32_t flags;
    union {
        double Float;
        int32_t Integer;
        char *String;
        struct php_object_s *Array;
        struct php_object_s *Object;
        uint8_t Boolean;
        void *Null;
        void *Reference;
    } data;
};

struct php_key_s {
    struct php_element_s elem;
    char *class;
    uint32_t flags;
};

struct php_slice_s {
    struct php_key_s *key;
    struct php_element_s *value;
};

struct php_object_s {
    uint32_t flags;
    uint32_t count;
    char *class;
    struct php_slice_s properties[];
};

typedef enum var_type php_type_t;
typedef struct php_element_s php_element_t;
typedef struct php_slice_s php_slice_t;
typedef struct php_object_s php_object_t;
typedef struct php_key_s php_key_t;

