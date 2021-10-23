#include <stdio.h>
#include <stdlib.h>
#include "thirdparty/json.h"
#include <assert.h>
#include <time.h>
#include "tzo.h"

Value *makeString(char *str)
{
    Value *value = malloc(sizeof *value);
    value->type = String;
    value->string_value = str;
    return value;
}

Value *makeNumber(float val)
{
    Value *value = malloc(sizeof *value);
    value->type = Number;
    value->number_value = val;
    return value;
}

void _push(Value val)
{
    stack[stackSize] = val;
    stackSize += 1;
}

Value _pop()
{
    Value val = stack[stackSize - 1];
    stackSize -= 1;
    return val;
}

Value _popS()
{
    Value val = _pop();
    assert(val.type == String);
    return val;
}

char *asString(Value val)
{
    if (val.type == String)
    {
        return val.string_value;
    }
    else
    {
        size_t needed = snprintf(NULL, 0, "%f", val.number_value) + 1;
        char *buffer = malloc(needed);
        sprintf(buffer, "%f", val.number_value);
        return buffer;
    }
}

void plus()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeNumber(a.number_value + b.number_value);
    _push(*c);
}

void min()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeNumber(a.number_value - b.number_value);
    _push(*c);
}

void mul()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeNumber(a.number_value * b.number_value);
    _push(*c);
}

void nop()
{
}

void pop()
{
    _pop();
}

void std_out()
{
    Value a = _pop();
    if (a.type == Number)
    {
        printf("%f", a.number_value);
    }
    if (a.type == String)
    {
        printf("%s", a.string_value);
    }
}

void concat()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeString(strcat(asString(a), asString(b)));
    _push(*c);
}

void rconcat()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeString(strcat(asString(a), asString(b)));
    _push(*c);
}

void charCode()
{
    Value a = _pop();
    size_t needed = snprintf(NULL, 0, "%c", (int)a.number_value) + 1;
    char *buffer = malloc(needed);
    sprintf(buffer, "%c", (int)a.number_value);
    Value *b = makeString(buffer);
    _push(*b);
}

void randInt()
{
    Value a = _pop();
    int i = rand() % (int)a.number_value;
    Value *b = makeNumber((float)i);
    _push(*b);
}

void eq()
{
    Value a = _pop();
    Value b = _pop();
    if (a.type != b.type)
    {
        _push(*makeNumber(0));
    }
    else if (a.type == String)
    {
        if (strcmp(a.string_value, b.string_value) == 0)
        {
            _push(*makeNumber(1));
        }
        else
        {
            _push(*makeNumber(0));
        }
    }
    else if (a.type == Number)
    {
        if (a.number_value == b.number_value)
        {
            _push(*makeNumber(1));
        }
        else
        {
            _push(*makeNumber(0));
        }
    }
}

void and ()
{
    Value a = _pop();
    Value b = _pop();
    if (a.type == Number && b.type == Number)
    {
        if (a.number_value == 0 || b.number_value == 0)
        {
            _push(*makeNumber(0));
            return;
        }
        else
        {
            _push(*makeNumber(1));
            return;
        }
    }
    _push(*makeNumber(0));
}

struct json_value_s *loadFileGetJSON(char *filename)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *inputjson = malloc(fsize + 1);
    fread(inputjson, 1, fsize, f);
    fclose(f);

    inputjson[fsize] = 0;
    struct json_value_s *root = json_parse(inputjson, strlen(inputjson));
    return root;
}

void initProgramListFromJSONArray(struct json_array_s *array)
{
    program = malloc(array->length * sizeof *program);
    stack = malloc(TZO_MAX_STACK_SIZE * sizeof *stack);
    stackSize = 0;

    programSize = array->length;
    int piPointer = 0;
    for (struct json_array_element_s *elem = array->start; elem != NULL; elem = elem->next)
    {
        // set defaults:
        program[piPointer].type = invoke_function;
        program[piPointer].function_pointer = &nop;

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
                    program[piPointer].value = makeString(str);
                }
                if (json_value_as_number(oe->value) != NULL)
                {
                    program[piPointer].value = makeNumber(atof(json_value_as_number(oe->value)->number));
                }
            }
            if (0 == strcmp(oe->name->string, "functionName"))
            {
                bind_function("plus", &plus);
                bind_function("+", &plus);
                bind_function("min", &min);
                bind_function("-", &min);
                bind_function("mul", &mul);
                bind_function("*", &mul);
                bind_function("pop", &pop);
                bind_function("stdout", &std_out);
                bind_function("concat", &concat);
                bind_function("rconcat", &rconcat);
                bind_function("charCode", &charCode);
                bind_function("randInt", &randInt);
                bind_function("eq", &eq);
                bind_function("and", &and);
            }
        }
        piPointer += 1;
    }
}

void step()
{
    if (program[ppc].type == push_string)
    {
        //printf("->%s\n", program[ppc].value->string_value);
        _push(*makeString(program[ppc].value->string_value));
    }
    if (program[ppc].type == push_number)
    {
        //printf("->%f\n", program[ppc].value->number_value);
        _push(*makeNumber(program[ppc].value->number_value));
    }
    if (program[ppc].type == invoke_function)
    {
        //printf("->FUNC\n");
        program[ppc].function_pointer();
    }
    ppc += 1;
}

void run()
{
    running = true;
    while (running && ppc < programSize)
    {
        step();
    }
    running = false;
}
