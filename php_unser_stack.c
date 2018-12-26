#include <stdlib.h>
#include "php_unser_stack.h"

#include "debug.h"

/********************************************************************/

void free_php_unser_stack (php_unser_stack_t *stack) {
    while (stack->prev != NULL) stack = stack->prev;
    
    while (stack->next != NULL) {
        stack = stack->next;
        free(stack->prev);
    }

    free(stack);
    return;
}

/********************************************************************/

php_unser_stack_t *php_push_unser_stack (php_element_t *elem, php_unser_stack_t *stack) {

//    _debug("Pushing element onto stack", 0);
    if (stack == (void*)-1) return NULL;

    while (stack != NULL && stack->next != NULL)
        stack = stack->next;

    if (stack == NULL || stack->count == stack->size) {
//        _debug("stack is 0x%X", stack);
        php_unser_stack_t *newstack = calloc(1, sizeof(php_unser_stack_t) + (sizeof(struct php_element_s *) * _PHP_UNSER_STACK_SIZE));
        newstack->count = 0;
        newstack->size = _PHP_UNSER_STACK_SIZE;
        newstack->next = NULL;
        if (stack == NULL) {
            newstack->prev = NULL;
            newstack->offset = 0;
        } else {
            newstack->prev = stack;
            stack->next = newstack;
            newstack->offset = stack->offset + stack->count;
        }
        stack = newstack;
    }

    stack->elements[stack->count++] = elem;

    return stack;
}

/********************************************************************/

php_element_t *php_find_element_in_stack(php_unser_stack_t *stack, int ref_num) {
//    _debug("Finding element %u in stack", ref_num);
    
    while (ref_num < (stack->offset + 1)) {
        if (stack->prev == NULL) {
            _debug("ERROR: Broken reference, can't stride back to %u from block starting at offset %u", ref_num, stack->offset + 1);
            return NULL;
        }
        stack = stack->prev;
    }

    while (ref_num >= (stack->offset + stack->size + 1)) {
        if (stack->next == NULL || stack->count < stack->size) {
            _debug("ERROR: Broken Reference %u", ref_num);
            return NULL;
        }
        stack = stack->next;
    }

    return stack->elements[ref_num - stack->offset - 1];
}

/********************************************************************/

