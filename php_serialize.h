#pragma once
#include "php_common.h"
#include "php_unser_stack.h"
#include <stdint.h>

php_element_t *php_unserialize (char *input, int len, char **new_offset, php_element_t **target, php_unser_stack_t *stack);

char *_null_term_to (char *input, char delim, int len);
int php_read_integer (char *input, char **new_offset, int len);
double php_read_float (char *input, char **new_offset, int len);
int php_read_string (char *input, char **dest, char **new_offset, int len, uint32_t *flags);

int php_read_key (char *input, php_key_t **dest, char **new_offset, int len);
int php_read_array (char *input, php_object_t **dest, char **new_offset, int len, php_unser_stack_t *stack);
int php_read_object (char *input, php_object_t **dest, char **new_offset, int len, php_unser_stack_t *stack);
