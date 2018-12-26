#ifdef USE_FASTCGI
    #include <fcgi_stdio.h>
#else 
    #include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "php_common.h"
#include "php_helpers.h"

#include "debug.h"

/********************************************************************/

void php_clear_printed (php_element_t *elem) {
    if (!(elem->flags & _PHP_FL_PRINTED)) return;
    elem->flags &= ~_PHP_FL_PRINTED;
    if (elem->type == Array || elem->type == Object) {
        int i;
        for (i = 0; i < elem->data.Array->count; i++)
            php_clear_printed(elem->data.Array->properties[i].value);
    }
    return;
}

/********************************************************************/

void php_free_element (php_element_t *elem) {
//    _debug("freeing php element (refs: %u)", elem->refs);
    if (--(elem->refs)) return;
    
    if (elem->type == Object || elem->type == Array)
        php_free_object(elem);
    else if ((elem->type > Reference) && (elem->type < Unknown) && (elem->flags & _PHP_FL_MALLOCED)) {
//        _debug("--> freeing data of type %u", elem->type);
        free(elem->data.String);
    }

//    _debug("--> freeing element structure", 0);
    free(elem);

//    _debug("<-- done freeing", 0);
    return;
}

/********************************************************************/

void php_free_object (php_element_t *elem) {
    int i;
    php_object_t *obj = elem->data.Object;

//    _debug("freeing php object of class '%s'", obj->class);

    if (obj->flags & _PHP_FL_MALLOCED && obj->class != NULL) {
        _debug("--> freeing class name '%s'", obj->class);
        free(obj->class);
        obj->class = NULL;
        obj->flags &= ~_PHP_FL_MALLOCED;
    }

    for (i = 0; i < obj->count; i++) {
        php_slice_t *slice = &(obj->properties[i]);
        #ifdef _DEBUG_
            if (slice->key->elem.type == Integer) {
//                _debug("--> freeing property with key %u", slice->key->elem.data.Integer);
            } else {
//                _debug("--> freeing property with key '%s'", slice->key->elem.data.String);
            }
        #endif
        if (slice->key->flags & _PHP_FL_MALLOCED && slice->key->class != NULL) {
//            _debug("--> freeing key's class name '%s'", slice->key->class);
            free(slice->key->class);
            slice->key->flags &= ~_PHP_FL_MALLOCED;
        }
        php_free_element((php_element_t*)slice->key);
        php_free_element(slice->value);
    }

    return;
}

/********************************************************************/

php_element_t *php_isset (php_element_t *elem, const char *name) {
    char buf[30];
    buf[0] = '\0';
    const char *tmp = name;
    int len, i, klen;

//    _debug("checking if '%s' isset", name);

    if (elem->type != Object && elem->type != Array) {
        if (tmp[0] == '\0') return elem;
//        _debug("element is not an object or array!", 0);
        return NULL;
    }

    while (name[0] != '\0') {
        len = 0;
        if (name[0] == '-' && name[1] == '>') name += 2;
        else if (name[0] == ']') name++;
        if (name[0] == '[') name++;

        if (name[0] == '\0') break;

        tmp = name;

//        _debug("remaining keys are '%s'", name);
        for (; (name[0] != '\0'); name++, len++)
            if ((name[0] == ']') || (name[0] == '-' && name[1] == '>')) break;

        memcpy(buf, tmp, len);
        buf[len] = '\0';

        /* Numerical key */
        if (tmp[0] > 47 && tmp[0] < 58) {
            int k = atoi(buf);
//            _debug("finding numerical key %u at '%s'", k, tmp);
            if (k > elem->data.Object->count) {
//                _debug("--> beyond count!", 0);
                return NULL;
            }
//            _debug("--> found element...", 0);
            elem = elem->data.Object->properties[k].value;
            continue;
        }

        /* String key */
//        _debug("finding string key '%s'", buf);
        for (i = 0; i < elem->data.Object->count; i++) {
            if (elem->data.Object->properties[i].key->elem.type != String) {
//                _debug("element does not have a string key (%u)", elem->data.Object->properties[i].key->elem.data.Integer);
                continue;
            }
            klen = strlen(elem->data.Object->properties[i].key->elem.data.String);
//            _debug("--> comparing against %u: '%s'", klen, elem->data.Object->properties[i].key->elem.data.String);
            if (klen != len) continue;
            if (!memcmp(elem->data.Object->properties[i].key->elem.data.String, buf, klen)) {
//                _debug("--> Found element!", 0);
                elem = elem->data.Object->properties[i].value;
                i = -1;
                break;
            }
        }

        if (i == -1) continue;

        if (i >= elem->data.Object->count) {
            _debug("NOTE:  Could not find element with key '%s'", tmp);
            return NULL;
        }
    }

//    _debug("found target element!", 0);
    return elem;
}

/********************************************************************/

void php_print_r (php_element_t *elem, int initial, int tab, int no_first_tab, FILE *out) {
//    _debug("Entered print_r", 0);
    int nohead = 0;

    char *tabs = malloc(tab+1);
    memset(tabs, '\t', tab);
    tabs[tab] = '\0';

    if (!no_first_tab) 
        fprintf(out, "%s", tabs);

    if (elem->flags & _PHP_FL_PRINTED)
        fprintf(out, "*** RECURSIVE ***\n");
    else {
    
        elem->flags |= _PHP_FL_PRINTED;
    
        switch (elem->type) {
            case (Integer):
                fprintf(out, "%d\n", elem->data.Integer);
                break;
            case (Float):
                fprintf(out, "%f\n", elem->data.Float);
                break;
            case (String):
                fprintf(out, "\"%s\"\n", elem->data.String);
                break;
            case (Object):
                fprintf(out, "Object (%s) => {\n", elem->data.Object->class);
                nohead = 1;
            case (Array):
                if (!nohead)
                    fprintf(out, "Array => {\n");
    
                int i;
                for (i = 0; i < elem->data.Array->count; i++) {
                    if (elem->data.Array->properties[i].key->elem.type == String) {
                        if (elem->data.Array->properties[i].key->flags & _PHP_FL_PRIVATE)
                            fprintf(out, "%s\t[%s:%s] => ", tabs,
                                elem->data.Array->properties[i].key->class,
                                elem->data.Array->properties[i].key->elem.data.String
                            );
                        else if (elem->data.Array->properties[i].key->flags & _PHP_FL_PROTECTED)
                            fprintf(out, "%s\t[protected:%s] => ", tabs,
                                elem->data.Array->properties[i].key->elem.data.String
                            );
                        else
                            fprintf(out, "%s\t[%s] => ", tabs,
                                elem->data.Array->properties[i].key->elem.data.String
                            );
                    } else 
                        fprintf(out, "%s\t[%d] => ", tabs, elem->data.Array->properties[i].key->elem.data.Integer);
    
                    php_print_r(elem->data.Array->properties[i].value, 0, tab+1, 1, out);
                }
                fprintf(out, "%s}\n", tabs);
                break;
            case (Boolean):
                fprintf(out, "%s\n", (elem->data.Boolean)?"TRUE":"FALSE");
                break;
            case (Null):
                fprintf(out, "NULL\n");
                break;
            case (Reference):
            case (Unknown):
            default:
                fprintf(out, "Unknown => '%s'\n", elem->data.Reference);
                break;
        }
    }

    if (initial)
        php_clear_printed(elem);

    free(tabs);
    return;
}

/********************************************************************/

