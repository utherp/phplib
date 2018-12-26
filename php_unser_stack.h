#pragma once
#include "php_common.h"

#define _PHP_UNSER_STACK_SIZE 64

struct php_unser_stack_s {
    struct php_unser_stack_s *prev;
    struct php_unser_stack_s *next;
    unsigned int offset;
    unsigned int count;
    unsigned int size;
    struct php_element_s *elements[];
};
typedef struct php_unser_stack_s php_unser_stack_t;

void free_php_unser_stack (php_unser_stack_t *stack);
php_unser_stack_t *php_push_unser_stack (php_element_t *elem, php_unser_stack_t *stack);
php_element_t *php_find_element_in_stack(php_unser_stack_t *stack, int ref_num);
