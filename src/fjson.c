#include <string.h>
#include <stdlib.h>

#include <fjson/fjson.h>


fjson_t* fjson_new(void)
{
    fjson_t *fjson = calloc(1, sizeof(fjson_t));
    fjson->state = FJSON_STATE_ELEMENT;
    fjson->el = calloc(1, sizeof(fjson_element_t));
    return fjson;
}

void fjson_free(fjson_t *fjson)
{

    if (!fjson)
        return;

    if (fjson->child)
        fjson_free(fjson->child);

    free(fjson->buf);
    free(fjson);
}

void fjson_free_element(fjson_element_t *el)
{

    fjson_pair_t *cur_pair = 0;
    fjson_array_t *array = 0;

    if (!el)
        return;


    switch (el->type) {
    case FJSON_TYPE_OBJECT:

        cur_pair = el->pairs;
        while (cur_pair) {
            fjson_pair_t *next = cur_pair->next;
            fjson_free_element(cur_pair->key);
            fjson_free_element(cur_pair->value);
            free(cur_pair);
            cur_pair = next;
        }
        break;

    case FJSON_TYPE_ARRAY:

        array = el->array;
        while (array) {
            fjson_array_t *next = array->next;
            fjson_free_element(array->el);
            free(array);
            array = next;
        }
        break;

    case FJSON_TYPE_STRING:
        free(el->str);
        break;

    default:
        // Do nothing
        break;
    }

    free(el);
}

fjson_element_t* fjson_get_value_by_key(fjson_element_t *obj, const char *key)
{
    fjson_pair_t *cur_pair;

    if (obj->type != FJSON_TYPE_OBJECT)
        return 0;

    cur_pair = obj->pairs;

    for(; cur_pair; cur_pair = cur_pair->next){
        if (strcmp(cur_pair->key->str, key) == 0)
            return cur_pair->value;
    }

    return 0;
}

static fjson_pair_t* pair_new(fjson_element_t *key, fjson_element_t *value)
{

    fjson_pair_t *pair = calloc(1, sizeof(fjson_pair_t));

    pair->key = key;
    pair->value = value;

    return pair;
}

static fjson_array_t* array_new(fjson_element_t *el)
{

    fjson_array_t *array = calloc(1, sizeof(fjson_array_t));

    array->el = el;

    return array;
}

static fjson_pair_t* get_last_pair(fjson_element_t *el)
{

    fjson_pair_t *last_pair = el->pairs;

    if (!last_pair)
        return 0;

    while (last_pair->next)
        last_pair = last_pair->next;

    return last_pair;
}

static fjson_array_t* get_last_array(fjson_element_t *el)
{

    fjson_array_t *last_array = el->array;

    if (!last_array)
        return 0;

    while (last_array->next)
        last_array = last_array->next;

    return last_array;

}

static void add_pair(fjson_element_t *el, fjson_pair_t *pair)
{

    pair->next = NULL;

    if (!el->pairs) {

        el->pairs = pair;

    } else {

        fjson_pair_t *last_pair = get_last_pair(el);
        last_pair->next = pair;

    }

}

static int is_whitespace(char byte)
{
    return (byte == ' ' || byte == '\t' || byte == '\n' || byte == '\v' || byte == '\f' || byte == '\r' );
}

static int is_number(char byte)
{
    return (byte >= 0x30 && byte <= 0x39);
}

static void write_buf(fjson_t *fjson, char byte)
{

    fjson->buf = realloc(fjson->buf, fjson->bi + 1);
    fjson->buf[fjson->bi] = byte;
    fjson->bi++;

}

static void reset_buf(fjson_t *fjson)
{

    free(fjson->buf);
    fjson->buf = 0;
    fjson->bi = 0;

}

static int state_object_pair(fjson_t *fjson, char byte)
{

    if (is_whitespace(byte))
        return 0;

    if (byte == '"') {
        fjson->state = FJSON_STATE_OBJECT_KEY;
        return 0;
    } else if (byte == '}')
        return 1;
    else
        return -1;

}

static int state_object_key(fjson_t *fjson, char byte)
{

    int r;

    if (!fjson->child) {
        fjson->child = fjson_new();
        fjson->child->father = fjson;
        fjson_putbyte(fjson->child, '"');
    }

    r = fjson_putbyte(fjson->child, byte);

    if (r == 0) // Still parsing
        return 0;

    if (r == 1) {
        fjson->state = FJSON_STATE_OBJECT_KEY_PARSED;
        add_pair(fjson->el, pair_new(fjson->child->el, 0));
    }

    fjson_free(fjson->child);
    fjson->child = NULL;

    if (r == -1)
        return -1;

    return 0;

}

static int state_object_value(fjson_t *fjson, char byte)
{
    int parsed_obj = 0;
    int r;

    if (!fjson->child) { // Still not parsing the value

        if (is_whitespace(byte)) // If the byte is not a blank char the value has started
            return 0;

        fjson->child = fjson_new();
        fjson->child->father = fjson;

    }

    r = fjson_putbyte(fjson->child, byte);

    if (r == 0) // Still parsing
        return 0;

    if (r == 1) { // Successful parsing
        fjson_pair_t *last_pair = get_last_pair(fjson->el);
        last_pair->value = fjson->child->el;
        fjson->state = FJSON_STATE_OBJECT_AFTER_VALUE;

        if (fjson->child->el->type == FJSON_TYPE_OBJECT)
            parsed_obj = 1;

    }

    fjson_free(fjson->child);
    fjson->child = NULL;

    if (r == -1) // Parsing failed
        return -1;

    // r == 1, successful parsing
    // fjson->state is FJSON_STATE_OBJECT_AFTER_VALUE
    if (byte == ',' || (byte == '}' && !parsed_obj) ) {
        return fjson_putbyte(fjson, byte);
    }

    return 0;
}

static int state_array_value(fjson_t *fjson, char byte)
{
    int parsed_array = 0;
    int r;

    if (!fjson->child) {

        if (is_whitespace(byte))
            return 0;
        else if (byte == ']')
            return 1;

        fjson->child = fjson_new();
        fjson->child->father = fjson;

    }

    r = fjson_putbyte(fjson->child, byte);

    if (r == 0) // Still parsing
        return 0;

    if (r == 1) { // Successful parsing

        if (!fjson->el->array) {
            fjson->el->array = array_new(fjson->child->el);
        } else {
            fjson_array_t *last_array = get_last_array(fjson->el);
            last_array->next = array_new(fjson->child->el);
        }

        fjson->state = FJSON_STATE_ARRAY_AFTER_VALUE;

        if (fjson->child->el->type == FJSON_TYPE_ARRAY)
            parsed_array = 1;

    }

    fjson_free(fjson->child);
    fjson->child = NULL;

    if (r == -1) // Parsing failed
        return -1;

    // r == 1, successful parsing
    if (byte == ',' || (byte == ']' && !parsed_array) )
        return fjson_putbyte(fjson, byte);

    return 0;
}

static int state_element(fjson_t *fjson, char byte)
{

    switch (byte) {

    case '{':
        fjson->state = FJSON_STATE_OBJECT_PAIR;
        fjson->el->type = FJSON_TYPE_OBJECT;
        return 0;
        break;

    case '[':
        fjson->state = FJSON_STATE_ARRAY_VALUE;
        fjson->el->type = FJSON_TYPE_ARRAY;
        return 0;
        break;

    case '"':
        fjson->state = FJSON_STATE_STRING;
        fjson->el->type = FJSON_TYPE_STRING;
        return 0;
        break;

    case 't':
    case 'f':
        fjson->state = FJSON_STATE_BOOLEAN;
        fjson->el->type = FJSON_TYPE_BOOLEAN;
        write_buf(fjson, byte);
        return 0;
        break;

    case 'n':
        fjson->state = FJSON_STATE_NULL;
        fjson->el->type = FJSON_TYPE_NULL;
        write_buf(fjson, byte);
        return 0;
        break;

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        fjson->state = FJSON_STATE_NUMBER;
        fjson->el->type = FJSON_TYPE_NUMBER;
        write_buf(fjson, byte);
        return 0;
        break;

    default:
        return -1;
        break;

    }

}

static int state_object_key_parsed(fjson_t *fjson, char byte)
{

    if (is_whitespace(byte))
        return 0;

    if (byte == ':') { // Blank chars and ':' are the only accepted bytes
        fjson->state = FJSON_STATE_OBJECT_VALUE;
        return 0;
    } else
        return -1;

}

static int state_object_after_value(fjson_t *fjson, char byte)
{

    if (is_whitespace(byte)) { // Blank chars are ignored
        return 0;
    } else if (byte == ',') { // ',' means that we can wait for another pair
        fjson->state = FJSON_STATE_OBJECT_PAIR;
        return 0;
    } else if (byte == '}') { // End of the object
        return 1;
    } else {    // Any other char is considered error
        return -1;
    }

}

static int state_array_after_value(fjson_t *fjson, char byte)
{
    if (is_whitespace(byte)) {
        return 0;
    } else if (byte == ',') {
        fjson->state = FJSON_STATE_ARRAY_VALUE;
        return 0;
    } else if (byte == ']') {
        return 1;
    } else {
        return -1;
    }
}

static int state_string(fjson_t *fjson, char byte)
{
    if (byte == '"') {
        write_buf(fjson, '\0');
        fjson->el->str = fjson->buf;
        fjson->buf = NULL;
        fjson->bi = 0;
        return 1;
    } else if (byte == '\\') {
        fjson->state = FJSON_STATE_SPEC_CHAR;
    } else
        write_buf(fjson, byte);

    return 0;
}

static int state_spec_char(fjson_t *fjson, char byte)
{
    switch (byte) {

    case 'b':
        write_buf(fjson, '\b');
        break;

    case 'f':
        write_buf(fjson, '\f');
        break;

    case 'n':
        write_buf(fjson, '\n');
        break;

    case 'r':
        write_buf(fjson, '\r');
        break;

    case 't':
        write_buf(fjson, '\t');
        break;

    case '"':
    case '\\':
    case '/':
        write_buf(fjson, byte);
        break;

    default:
        return -1;
        break;
    }

    fjson->state = FJSON_STATE_STRING; // Return to normal string state

    return 0;
}

static int state_number(fjson_t *fjson, char byte)
{
    if (is_number(byte) || byte == '.') {
        write_buf(fjson, byte);
    } else if (is_whitespace(byte) || byte == ',' || byte == '}' || byte == ']') {
        write_buf(fjson, '\0');
        fjson->el->num = strtod(fjson->buf, 0);
        reset_buf(fjson);
        return 1;
    } else
        return -1;

    return 0;
}

static int state_boolean(fjson_t *fjson, char byte)
{
    write_buf(fjson, byte);

    if (fjson->bi < 4) // Still parsing
        return 0;

    if (fjson->bi > 5) // Too much data
        return -1;

    if (fjson->bi == 4 && strncmp(fjson->buf, "true", 4) == 0) {
        fjson->el->bool_val = 1;
        reset_buf(fjson);
        return 1;
    } else if (fjson->bi == 5 && strncmp(fjson->buf, "false", 5) == 0) {
        fjson->el->bool_val = 0;
        reset_buf(fjson);
        return 1;
    }

    return 0;
}

static int state_null(fjson_t *fjson, char byte)
{
    write_buf(fjson, byte);
    if (fjson->bi < 4) // Still parsing
        return 0;

    reset_buf(fjson);

    return 1;
}

int fjson_putbyte(fjson_t *fjson, char byte)
{
    switch (fjson->state) {

    case FJSON_STATE_ELEMENT:
        return state_element(fjson, byte);
        break;

    case FJSON_STATE_OBJECT_PAIR:
        return state_object_pair(fjson, byte);
        break;

    case FJSON_STATE_OBJECT_KEY:
        return state_object_key(fjson, byte);
        break;

    case FJSON_STATE_OBJECT_KEY_PARSED:
        return state_object_key_parsed(fjson, byte);
        break;

    case FJSON_STATE_OBJECT_VALUE: {

        int r = state_object_value(fjson, byte);
        if (!fjson->father) {
            return (r == -1) ? -1 : 0;
        } else
            return r;

        break;
    }

    case FJSON_STATE_OBJECT_AFTER_VALUE:
        return state_object_after_value(fjson, byte);
        break;

    case FJSON_STATE_ARRAY_VALUE:
        return state_array_value(fjson, byte);
        break;

    case FJSON_STATE_ARRAY_AFTER_VALUE:
        return state_array_after_value(fjson, byte);
        break;

    case FJSON_STATE_STRING:
        return state_string(fjson, byte);
        break;

    case FJSON_STATE_SPEC_CHAR:
        return state_spec_char(fjson, byte);
        break;

    case FJSON_STATE_NUMBER:
        return state_number(fjson, byte);
        break;

    case FJSON_STATE_BOOLEAN:
        return state_boolean(fjson, byte);
        break;

    case FJSON_STATE_NULL:
        return state_null(fjson, byte);
        break;

    default:
        return -1;
        break;

    }

}

int fjson_putbuf(fjson_t *fjson, char *buf, size_t len)
{
    size_t i;
    int r;

    for(i=0; i<len; i++){
        if ((r=fjson_putbyte(fjson, buf[i])) != 0){
            return r;
        }
    }
    return 0;
}
