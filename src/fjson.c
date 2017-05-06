#include <ctype.h>
#include <string.h>

#include <fjson/fjson.h>


fjson_t* fjson_new(){
    fjson_t *fjson = calloc( 1, sizeof(fjson_t) );
    fjson->state = FJSON_STATE_ELEMENT;
    fjson->el = calloc( 1, sizeof(fjson_element_t) );
    return fjson;
}

void fjson_free( fjson_t *fjson, int free_el ){

    if( !fjson )
        return;

    // TODO: FREE EL PLZ
    if( fjson->sub )
        fjson_free( fjson->sub, free_el );

    free( fjson->buf );
    free( fjson );
}

fjson_pair_t* fjson_pair_new( fjson_element_t *key, fjson_element_t *value ){

    fjson_pair_t *pair = calloc( 1, sizeof(fjson_pair_t) );

    pair->key = key;
    pair->value = value;

    return pair;
}

fjson_array_t* fjson_array_new( fjson_element_t *el ){

    fjson_array_t *array = calloc( 1, sizeof(fjson_array_t) );

    array->el = el;

    return array;
}

fjson_pair_t* fjson_get_last_pair( fjson_element_t *el ){

    fjson_pair_t *last_pair = el->pairs;

    if( !last_pair )
        return 0;

    while( last_pair->next )
        last_pair = last_pair->next;

    return last_pair;
}

fjson_array_t* fjson_get_last_array( fjson_element_t *el ){

    fjson_array_t *last_array = el->array;

    if( !last_array )
        return 0;

    while( last_array->next )
        last_array = last_array->next;

    return last_array;

}

void fjson_add_pair( fjson_element_t *el, fjson_pair_t *pair ){

    pair->next = NULL;

    if( !el->pairs ){

        el->pairs = pair;

    } else {

        fjson_pair_t *last_pair = fjson_get_last_pair( el );
        last_pair->next = pair;

    }

}

int fjson_is_blank( char byte ){
    return (byte == ' ' || byte == '\n' || byte == '\t' || byte == '\r' );
}

void fjson_putbyte_buf( fjson_t *fjson, char byte ){

    fjson->buf = realloc( fjson->buf, fjson->bi + 1 );
    fjson->buf[fjson->bi] = byte;
    fjson->bi++;

}

void fjson_reset_buf( fjson_t *fjson ){

    free( fjson->buf );
    fjson->buf = 0;
    fjson->bi = 0;

}

int fjson_state_object_pair( fjson_t *fjson, char byte ){

    if( fjson_is_blank( byte ) )
        return 0;

    if( byte == '"' ){
        fjson->state = FJSON_STATE_OBJECT_KEY;
        return 0;
    } else if( byte == '}' )
        return 1;
    else
        return -1;

    return -1;
}

int fjson_state_object_key( fjson_t *fjson, char byte ){

    int r;

    if( !fjson->sub ){
        fjson->sub = fjson_new();
        fjson_putbyte( fjson->sub, '"' );
    }

    r = fjson_putbyte( fjson->sub, byte );

    if( r == 0 ) // Still parsing
        return 0;

    if( r == 1 ){
        fjson->state = FJSON_STATE_OBJECT_KEY_PARSED;
        fjson_add_pair( fjson->el, fjson_pair_new( fjson->sub->el, 0 ) );
    }

    fjson_free( fjson->sub, 0 );
    fjson->sub = NULL;

    if( r == -1 )
        return -1;

    return 0;

}

int fjson_state_object_value( fjson_t *fjson, char byte ){

    if( !fjson->sub ){ // Still not parsing the value

        if( fjson_is_blank( byte ) ) // If the byte is not a blank char the value has started
            return 0;

        fjson->sub = fjson_new();

    }

    if( fjson->sub ){

        int r = fjson_putbyte( fjson->sub, byte );

        if( r == 0 ) // Still parsing
            return 0;

        if( r == 1 ){ // Successful parsing
            fjson_pair_t *last_pair = fjson_get_last_pair( fjson->el );
            last_pair->value = fjson->sub->el;
            fjson->state = FJSON_STATE_OBJECT_AFTER_VALUE;
        }

        fjson_free( fjson->sub, 0 );
        fjson->sub = NULL;

        if( r == -1 ) // Parsing failed
            return -1;

        // r == 1, successful parsing
        // fjson->state is FJSON_STATE_OBJECT_AFTER_VALUE
        if( byte == ',' || byte == '}' )
            return fjson_putbyte( fjson, byte );

    }

    return 0;
}

int fjson_state_array_value( fjson_t *fjson, char byte ){

    if( !fjson->sub ){

        if( fjson_is_blank(byte) )
            return 0;

        fjson->sub = fjson_new();

    }

    if( fjson->sub ){

        int r = fjson_putbyte( fjson->sub, byte );

        if( r == 0 ) // Still parsing
            return 0;

        if( r == 1 ){ // Successful parsing

            if( !fjson->el->array ){
                fjson->el->array = fjson_array_new( fjson->sub->el );
            } else {
                fjson_array_t *last_array = fjson_get_last_array( fjson->el );
                last_array->next = fjson_array_new( fjson->sub->el );
            }

            fjson->state = FJSON_STATE_ARRAY_AFTER_VALUE;

        }

        fjson_free( fjson->sub, 0 );
        fjson->sub = NULL;

        if( r == -1 ) // Parsing failed
            return -1;

        // r == 1, successful parsing
        if( byte == ',' || byte == ']' )
            return fjson_putbyte( fjson, byte );

    }

    return 0;

}

int fjson_putbyte( fjson_t *fjson, char byte ){

    switch( fjson->state ){

        case FJSON_STATE_ELEMENT:

            switch( byte ){

                case '{':
                    fjson->state = FJSON_STATE_OBJECT_PAIR;
                    fjson->el->type = FJSON_TYPE_OBJECT;
                    break;

                case '[':
                    fjson->state = FJSON_STATE_ARRAY_VALUE;
                    fjson->el->type = FJSON_TYPE_ARRAY;
                    break;

                case '"':
                    fjson->state = FJSON_STATE_STRING;
                    fjson->el->type = FJSON_TYPE_STRING;
                    break;

                case 't':
                case 'f':
                    fjson->state = FJSON_STATE_BOOLEAN;
                    fjson->el->type = FJSON_TYPE_BOOLEAN;
                    fjson_putbyte_buf( fjson, byte );
                    break;

                case 'n':
                    fjson->state = FJSON_STATE_NULL;
                    fjson->el->type = FJSON_TYPE_NULL;
                    fjson_putbyte_buf( fjson, byte );
                    break;

                case '+':
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
                    fjson_putbyte_buf( fjson, byte );
                    break;

                default:
                    return -1;
                    break;


            }
            break;

        case FJSON_STATE_OBJECT_PAIR:
            return fjson_state_object_pair( fjson, byte );
            break;

        case FJSON_STATE_OBJECT_KEY:
            return fjson_state_object_key( fjson, byte );
            break;

        case FJSON_STATE_OBJECT_KEY_PARSED:

            if( fjson_is_blank( byte ) )
                return 0;

            if( byte == ':' ) // Blank chars and ':' are the only accepted bytes
                fjson->state = FJSON_STATE_OBJECT_VALUE;
            else
                return -1;

            break;

        case FJSON_STATE_OBJECT_VALUE:
            return fjson_state_object_value( fjson, byte );
            break;


        case FJSON_STATE_OBJECT_AFTER_VALUE:

            if( byte == ',' )
                fjson->state = FJSON_STATE_OBJECT_PAIR;
            else if( byte == '}' )
                return 1;

            break;

        case FJSON_STATE_ARRAY_VALUE:
            return fjson_state_array_value( fjson, byte );
            break;

        case FJSON_STATE_ARRAY_AFTER_VALUE:

            if( byte == ',' )
                fjson->state = FJSON_STATE_ARRAY_VALUE;
            else if( byte == ']' )
                return 1;

            break;

        case FJSON_STATE_STRING:

            if( byte == '"' ){
                fjson_putbyte_buf( fjson, '\0' );
                fjson->el->str = fjson->buf;
                fjson->buf = NULL;
                fjson->bi = 0;
                return 1;
            } else
                fjson_putbyte_buf( fjson, byte );

            break;

        case FJSON_STATE_NUMBER:

            if( isdigit( byte ) ){
                fjson_putbyte_buf( fjson, byte );
            } else if( fjson_is_blank( byte ) || byte == ',' || byte == '}' || byte == ']' ){
                fjson_putbyte_buf( fjson, '\0' );
                fjson->el->num = strtoll( fjson->buf, 0, 0 );
                fjson_reset_buf( fjson );
                return 1;
            } else
                return -1;

            break;

        case FJSON_STATE_BOOLEAN:

            fjson_putbyte_buf( fjson, byte );
            if( fjson->bi < 4 ) // Still parsing
                return 0;

            if( fjson->bi > 5 ) // Too much data
                return -1;

            if( fjson->bi == 4 && strncmp( fjson->buf, "true", 4 ) == 0 ){
                fjson->el->bool_val = 1;
                fjson_reset_buf( fjson );
                return 1;
            } else if( fjson->bi == 5 && strncmp( fjson->buf, "false", 5 ) == 0 ){
                fjson->el->bool_val = 0;
                fjson_reset_buf( fjson );
                return 1;
            }

            break;

        case FJSON_STATE_NULL:

            fjson_putbyte_buf( fjson, byte );
            if( fjson->bi < 4 ) // Still parsing
                return 0;

            fjson_reset_buf( fjson );

            return 1;
            break;

        default:
            return -1;
            break;

    }

    return 0;
}