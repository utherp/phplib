#pragma once
#include "php_common.h"

#ifdef USE_FASTCGI
    #include <fcgi_stdio.h>
#else 
    #include <stdio.h>
#endif

void php_clear_printed (php_element_t *elem);

void php_print_r (php_element_t *elem, int initial, int tab, int no_first_tab, FILE *out);
void php_free_object (php_element_t *elem);
void php_free_element (php_element_t *elem);
php_element_t *php_isset (php_element_t *elem, const char *name);
