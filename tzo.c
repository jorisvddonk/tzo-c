#include <stdio.h>
#include <stdlib.h>
#include "thirdparty/json.h"

typedef enum
{
    push_string = 0,
    push_number = 1,
    invoke_function = 2,
} instructionType;

typedef struct
{
    instructionType type;
    char *string_value;
    float float_value;
    void (*function_pointer)();
} tzo_instr;



void add() {
    // todo    
}

int main()
{

    FILE *f = fopen("test.json", "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    char *inputjson = malloc(fsize + 1);
    fread(inputjson, 1, fsize, f);
    fclose(f);

    inputjson[fsize] = 0;
    printf("%s\n", inputjson);

    struct json_value_s *root = json_parse(inputjson, strlen(inputjson));

    struct json_array_s *array = json_value_as_array(root);
    printf("%lu\n", array->length);

    tzo_instr *program = malloc(array->length * sizeof *program);
    int programSize = array->length;
    int piPointer = 0;
    for (struct json_array_element_s *elem = array->start; elem != NULL; elem = elem->next)
    {
        struct json_object_s *obj = json_value_as_object(elem->value);
        for (struct json_object_element_s *oe = obj->start; oe != NULL; oe = oe->next)
        {
            if (0 == strcmp(oe->name->string, "type"))
            {
                if (0 == strcmp(json_value_as_string(oe->value)->string, "push-number-instruction"))
                {
                    program[piPointer].type = push_number;
                }
                if (0 == strcmp(json_value_as_string(oe->value)->string, "push-string-instruction"))
                {
                    program[piPointer].type = push_string;
                }
                if (0 == strcmp(json_value_as_string(oe->value)->string, "invoke-function-instruction"))
                {
                    program[piPointer].type = invoke_function;
                }
            }
            if (0 == strcmp(oe->name->string, "value"))
            {
                if (json_value_as_string(oe->value) != NULL)
                {
                    struct json_string_s *s = json_value_as_string(oe->value);
                    char *str = (char *)malloc(s->string_size);
                    strcpy(str, s->string);
                    program[piPointer].string_value = str;
                }
                if (json_value_as_number(oe->value) != NULL)
                {
                    program[piPointer].float_value = atof(json_value_as_number(oe->value)->number);
                }
            }
            if (0 == strcmp(oe->name->string, "functionName"))
            {
                if (0 == strcmp(json_value_as_string(oe->value)->string, "plus"))
                {
                    program[piPointer].function_pointer = &add;
                }
            }
        }
        piPointer += 1;
    }

    for (int i = 0; i < programSize; i++)
    {
        if (program[i].type == push_string)
        {
            printf("->%s\n", program[i].string_value);
        }
        if (program[i].type == push_number)
        {
            printf("->%f\n", program[i].float_value);
        }
    }

    return (0);
}