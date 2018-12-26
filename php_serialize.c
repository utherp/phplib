#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "php_serialize.h"

static const char *php_type_identifiers = "idbNROsa?";

#include "debug.h"

/********************************************************************/

php_element_t *php_unserialize (char *input, int len, char **new_offset, php_element_t **target, php_unser_stack_t *stack) {
    char *pos;
    enum var_type type;

    char first_stack = (stack==NULL)?1:0;

//    _debug("unserializing (%u) '%s'", len, input);

    for (type = 0; type < _PHP_MAX_TYPES && input[0] != php_type_identifiers[type]; type++);

    input += 2;
    len -= 2;

//    _debug("data type is %u", type);

    php_element_t *elem;

    if (type == Reference) {

//        _debug("--> type is Reference", 0);
        int ref_num = php_read_integer(input, &pos, len);
        input = pos;
//        _debug("----> to element %u in stack...", ref_num);

        elem = php_find_element_in_stack(stack, ref_num);

        elem->refs++;

//        _debug("----> found in stack an element of type %u, (%u references)", elem->type, elem->refs);

    } else {

        elem = (target!=NULL)?*target:calloc(1, sizeof(php_element_t));
        stack = php_push_unser_stack(elem, stack);

        elem->type = type;
        elem->flags = 0;
        elem->refs = 1;
    
//        _debug("-->remain (%u) '%s'", len, input);
        
        switch (elem->type) {
            case (Integer):
//                _debug("--> type is Integer", 0);
                elem->data.Integer = php_read_integer(input, &pos, len);
                elem->length = sizeof(uint32_t);
                break;
            case (Float):
//                _debug("--> type is Float", 0);
                elem->data.Float = php_read_float(input, &pos, len);
                elem->length = sizeof(double);
                break;
            case (String):
//                _debug("--> type is String", 0);
                elem->length = php_read_string(input, &(elem->data.String), &pos, len, &(elem->flags));
                break;
            case (Array):
//                _debug("--> type is Array", 0);
                elem->length = php_read_array(input, &(elem->data.Array), &pos, len, stack);
                break;
            case (Boolean):
//                _debug("--> type is Boolean", 0);
                pos = input + 2;
                elem->data.Boolean = (*input == '1')?1:0;
                elem->length = sizeof(uint8_t);
                break;
            case (Null):
//                _debug("--> type is Null", 0);
                pos = input;
                elem->data.Null = NULL;
                elem->length = sizeof(void *);
                break;
            case (Object):
//                _debug("--> type is Object", 0);
                elem->length = php_read_object(input, &(elem->data.Object), &pos, len, stack);
                break;
            case (Reference):
//                _debug("--> type is Reference", 0);
                break;
            case (Unknown):
            default:
//                _debug("--> type is Unknown", 0);
                elem->data.Reference = input - 2;
                elem->length = len;
                pos = input + len;
                break;
        }

    }

    if (new_offset != NULL)
        *new_offset = pos;

//    _debug("after read (%u) '%s'", len, pos);

    len -= (pos - input);
    if (len < 0) len = 0;

/*    
  #ifdef _DEBUG_
    if (len) {
        _debug("-->remain (%d) '%s'", len, pos);
    }
  #endif
*/

    if (first_stack)
        free_php_unser_stack(stack);

//    _debug("leaving unserialize", 0);

    return elem;
}

/********************************************************************/

char *_null_term_to (char *input, char delim, int len) {
    for (; *input != delim && *input != '\0' && len; input++, len--);
    *input = '\0';
    return input + 1;
}

/********************************************************************/

int php_read_integer (char *input, char **new_offset, int len) {

//    _debug("reading integer (%u) '%s'", len, input);

    char *tmp = _null_term_to(input, ';', len);
    if (new_offset != NULL)
        *new_offset = tmp;
    
    int v = atoi(input);
//    _debug("--> read int (%d), remain '%s'", v, tmp);

    return v;
}

/********************************************************************/

double php_read_float (char *input, char **new_offset, int len) {

//    _debug("reading float (%u) '%s'", len, input);

    if (new_offset != NULL) *new_offset = _null_term_to(input, ';', len);
    else _null_term_to(input, ';', len);
    return atof(input);
}

/********************************************************************/

int php_read_string (char *input, char **dest, char **new_offset, int len, uint32_t *flags) {

//    _debug("reading string (%u) '%s'", len, input);

    char *tmp = _null_term_to(input, ':', len);
    int s, size = atoi(input);
    len -= (tmp - input);
    len -= (size + 2);
    input = tmp + size + 1;

    *input = '\0';
    input+= 2;

    if (new_offset != NULL)
        *new_offset = input;

    if (*tmp == '"') tmp++;
    
    if (flags != NULL)
        for (s = 0; s < size; s++)
            if (!isprint((int)(tmp[s]))) {
                *flags |= _PHP_FL_BINARY;
                break;
            }

//    _debug("--> done reading string '%s', remain (%u) '%s'", tmp, len, input);
    *dest = tmp;
    return strlen(tmp);
}

/********************************************************************/

int php_read_key (char *input, php_key_t **dest, char **new_offset, int len) {

//    _debug("Reading key (%u) '%s'", len, input);
    php_key_t *key = calloc(1, sizeof(php_key_t));

    char *pos;
    php_unserialize (input, len, &pos, (php_element_t**)&key, (void*)-1);

    if (key->elem.type == String && key->elem.data.String[0] == '\0') {
        key->class = key->elem.data.String + 1;
        int clslen = strlen(key->class);
        key->elem.data.String = key->class + clslen + 1;
        key->elem.length -= clslen - 2;
        key->elem.flags &= ~_PHP_FL_BINARY;
        if (clslen == 1 && key->class[0] == '*') {
            key->class = "protected";
            key->flags |= _PHP_FL_PROTECTED;
        } else
            key->flags |= _PHP_FL_PRIVATE;
    } else 
        key->class = NULL;

    *dest = key;

    if (new_offset != NULL)
        *new_offset = pos;
    
    return key->elem.length;
}

/********************************************************************/

int php_read_array (char *input, php_object_t **dest, char **new_offset, int len, php_unser_stack_t *stack) {

//    _debug("reading array (%u) '%s'", len, input);

    char *tmp, *pos = _null_term_to(input, ':', len);
    int i = 0, size = atoi(input);
    len -= (pos - input);

//    _debug("allocating for array of %u elements", size);

    php_object_t *array = calloc(1, sizeof(php_object_t) + (sizeof(php_slice_t) * size));
    array->count = size;
    array->class = "Array";

    if (*pos == '{') pos++;

    while (*pos != '}' && len) {
//        _debug("##> Reading Key (%u) '%s'", len, pos);

        php_read_key(pos, &(array->properties[i].key), &tmp, len);
//        array->properties[i].key = php_unserialize(pos, len, &tmp);

        len -= (tmp - pos);
        pos = tmp;

        if (!len) continue;

//        _debug("##> Read Key, reading value (%u) '%s'", len, pos);
        
        array->properties[i].value = php_unserialize(pos, len, &tmp, NULL, stack);

        len -= (tmp - pos);
        pos = tmp;

        i++;
    }

    if (*pos == '}') {
        pos++;
        len--;
    }

    *dest = array;

    if (new_offset != NULL)
        *new_offset = pos;

    return array->count;
}

/********************************************************************/

int php_read_object (char *input, php_object_t **dest, char **new_offset, int len, php_unser_stack_t *stack) {

//    _debug("reading object (%u) '%s'", len, input);

    char *class;
    char *pos;
    php_read_string(input, &class, &pos, len, NULL);
    len -= (pos - input);

    input = pos;

    php_object_t *array;
    int size = php_read_array(input, &array, &pos, len, stack);

//    _debug("setting object's name '%s'", class);
    array->class = class;

    len -= (pos - input);

    if (new_offset != NULL)
        *new_offset = pos;

    *dest = array;

    return size;
}

/********************************************************************/

