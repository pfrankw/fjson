
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fjson/fjson.h>

void print_el(fjson_element_t *el)
{
    if (!el)
        return;

    switch (el->type) {

    case FJSON_TYPE_OBJECT: {

        fjson_pair_t *cur_pair = el->pairs;

        printf("{ ");

        while (cur_pair) {
            printf("\"%s\": ", cur_pair->key->str);
            print_el(cur_pair->value);
            cur_pair = cur_pair->next;
            if (cur_pair)
                printf(", ");
        }

        printf(" }");
        break;
    }

    case FJSON_TYPE_ARRAY: {

        fjson_array_t *cur = el->array;

        printf("[ ");

        while (cur) {
            print_el(cur->el);
            cur = cur->next;
            if (cur)
                printf(", ");
        }
        printf(" ]");
        break;
    }

    case FJSON_TYPE_STRING:
        printf("\"%s\"", el->str);
        break;

    case FJSON_TYPE_NULL:
        printf("null");
        break;

    case FJSON_TYPE_BOOLEAN:
        if (el->bool_val)
            printf("true");
        else
            printf("false");
        break;

    case FJSON_TYPE_NUMBER:
        printf("%.2f", el->num);
        break;

    default:
        break;
    }
}

int main(int argc, char **argv)
{

    int r = -1;
    fjson_t *fjson = fjson_new();
    FILE *fp = 0;
    char *json_buf = 0;
    size_t json_len = 0;

    fp = fopen(argv[1], "rb");
    if (!fp)
        goto cleanup;

    fseek(fp, 0, SEEK_END);
    json_len = ftell(fp);
    rewind(fp);

    json_buf = calloc(1, json_len);

    if (fread(json_buf, 1, json_len, fp) != json_len)
        goto cleanup;

    if (fjson_putbuf(fjson, json_buf, json_len) != 1)
        goto cleanup;

    print_el(fjson->el);


    r = 0;
cleanup:
    fjson_free_element(fjson->el);
    fjson_free(fjson);
    free(json_buf);
    if (fp) fclose(fp);
    return r;
}
