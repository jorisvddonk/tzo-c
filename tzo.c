#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "thirdparty/json.h"
#include "thirdparty/hashmap.h"
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

float asInt_f(Value val)
{
    if (fabsf(roundf(val.number_value) - val.number_value) <= 0.00001f)
    {
        return roundf(val.number_value);
    }
    return val.number_value;
}

char *asString(Value val)
{
    if (val.type == String)
    {
        return val.string_value;
    }
    else
    {
        if (fabsf(roundf(val.number_value) - val.number_value) <= 0.00001f)
        {
            size_t needed = snprintf(NULL, 0, "%i", (int)val.number_value) + 1;
            char *buffer = malloc(needed);
            sprintf(buffer, "%i", (int)val.number_value);
            return buffer;
        }
        else
        {
            size_t needed = snprintf(NULL, 0, "%f", val.number_value) + 1;
            char *buffer = malloc(needed);
            sprintf(buffer, "%f", val.number_value);
            return buffer;
        }
    }
}

void i_plus()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeNumber(a.number_value + b.number_value);
    _push(*c);
}

void i_min()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeNumber(a.number_value - b.number_value);
    _push(*c);
}

void i_mul()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeNumber(a.number_value * b.number_value);
    _push(*c);
}

void i_nop()
{
}

void i_br_close();

void i_br_open()
{
    int i = 1;
    int pc = ppc + 1;
    while (pc < programSize)
    {
        if (program[pc].type == invoke_function && program[pc].function_pointer == &i_br_open)
        {
            i += 1;
        }
        else if (program[pc].type == invoke_function && program[pc].function_pointer == &i_br_close)
        {
            i -= 1;
            if (i == 0)
            {
                // found it!
                ppc = pc;
                return;
            }
        }
        pc += 1;
    }
}

void i_br_close()
{
    // nop
}

void i_pop()
{
    _pop();
}

void i_stdout()
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

void i_concat()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeString(strcat(asString(a), asString(b)));
    _push(*c);
}

void i_rconcat()
{
    Value a = _pop();
    Value b = _pop();
    Value *c = makeString(strcat(asString(b), asString(a)));
    _push(*c);
}

void i_charCode()
{
    Value a = _pop();
    size_t needed = snprintf(NULL, 0, "%c", (int)a.number_value) + 1;
    char *buffer = malloc(needed);
    sprintf(buffer, "%c", (int)a.number_value);
    Value *b = makeString(buffer);
    _push(*b);
}

void i_randInt()
{
    Value a = _pop();
    int i = rand() % (int)a.number_value;
    Value *b = makeNumber((float)i);
    _push(*b);
}

void i_eq()
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

void i_and()
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

void i_dup()
{
    Value a = _pop();
    if (a.type == String)
    {
        _push(*makeString(a.string_value));
        _push(a);
    }
    else if (a.type == Number)
    {
        _push(*makeNumber(a.number_value));
        _push(a);
    }
}

void i_gt()
{
    Value a = _pop();
    Value b = _pop();
    if (a.type == Number && b.type == Number)
    {
        if (a.number_value > b.number_value)
        {
            _push(*makeNumber(1));
            return;
        }
        else
        {
            _push(*makeNumber(0));
            return;
        }
    }
    _push(*makeNumber(0));
}

void i_lt()
{
    Value a = _pop();
    Value b = _pop();
    if (a.type == Number && b.type == Number)
    {
        if (a.number_value < b.number_value)
        {
            _push(*makeNumber(1));
            return;
        }
        else
        {
            _push(*makeNumber(0));
            return;
        }
    }
    _push(*makeNumber(0));
}

void i_jgz()
{
    Value a = _pop();
    assert(("jgz: value must be number", a.type == Number));
    if (asInt_f(a) > 0)
    {
        ppc += 1;
    }
}

void i_jz()
{
    Value a = _pop();
    assert(("jz: value must be number", a.type == Number));
    if (asInt_f(a) == 0)
    {
        ppc += 1;
    }
}

void i_or()
{
    Value a = _pop();
    Value b = _pop();
    if (a.type == Number && b.type == Number)
    {
        if (a.number_value == 0 && b.number_value == 0)
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

void i_not()
{
    Value a = _pop();
    if (a.type == Number)
    {
        if (a.number_value == 0)
        {
            _push(*makeNumber(1));
            return;
        }
        else
        {
            _push(*makeNumber(0));
            return;
        }
    }
    _push(*makeNumber(0));
}

void i_ppc()
{
    _push(*makeNumber((float)ppc));
}

void i_stacksize()
{
    _push(*makeNumber((float)stackSize));
}

void i_pause()
{
    running = false;
}

void i_exit()
{
    running = false;
    exited = true;
}

void i_goto()
{
    Value a = _pop();
    if (a.type == Number)
    {
        int i = (int)(asInt_f(a)) - 1;
        ppc = i;
    }
    else if (a.type == String)
    {
        int i = hashmap_get(&labelmap, a.string_value, strlen(a.string_value));
        ppc = i - 1;
    }
}

void i_setContext()
{
    Value a = _pop();
    Value b = _pop();
    char *key = a.string_value;
    if (b.type == String)
    {
        hashmap_put(&context, key, strlen(key), makeString(b.string_value));
    }
    else if (b.type == Number)
    {
        hashmap_put(&context, key, strlen(key), makeNumber(b.number_value));
    }
}

void i_getContext()
{
    Value a = _pop();
    char *key = a.string_value;
    Value *val = (Value *)hashmap_get(&context, a.string_value, strlen(a.string_value));
    _push(*val);
}

void i_hasContext()
{
    Value a = _pop();
    char *key = a.string_value;
    void *const element = hashmap_get(&context, a.string_value, strlen(a.string_value));
    if (element == NULL)
    {
        _push(*makeNumber(0));
    }
    else
    {
        _push(*makeNumber(1));
    }
}

void i_delContext()
{
    Value a = _pop();
    char *key = a.string_value;
    hashmap_remove(&context, a.string_value, strlen(a.string_value));
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

void initRuntime()
{
    const unsigned initial_size = 64;
    if (0 != hashmap_create(initial_size, &labelmap))
    {
        assert(("failed to create label hashmap", 0));
    }
    if (0 != hashmap_create(initial_size, &context))
    {
        assert(("failed to create context hashmap", 0));
    }
    if (0 != hashmap_create(initial_size, &foreignFunctions))
    {
        assert(("failed to create foreignFunctions hashmap", 0));
    }
}

void initLabelMapFromJSONObject(struct json_object_s *obj)
{
    for (struct json_object_element_s *s = obj->start; s != NULL; s = s->next)
    {
        const char *key = s->name->string;
        if (json_value_as_number(s->value) != NULL)
        {
            struct json_number_s *n = json_value_as_number(s->value);
            float f = strtof(n->number, NULL);
            int i = (int)f;
            hashmap_put(&labelmap, key, strlen(key), i);
        }
    }
}

void initProgramListFromJSONArray(struct json_array_s *array)
{
    program = malloc(array->length * sizeof(*program));
    stack = malloc(TZO_MAX_STACK_SIZE * sizeof(*stack));
    stackSize = 0;

    programSize = array->length;
    int piPointer = 0;
    for (struct json_array_element_s *elem = array->start; elem != NULL; elem = elem->next)
    {

        // set defaults:
        program[piPointer].type = invoke_function;
        program[piPointer].function_pointer = &i_nop;

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
            if (0 == strcmp(oe->name->string, "label"))
            {
                char *key = json_value_as_string(oe->value)->string;
                hashmap_put(&labelmap, key, strlen(key), piPointer);
            }
            if (0 == strcmp(oe->name->string, "value"))
            {
                if (json_value_as_string(oe->value) != NULL)
                {
                    struct json_string_s *s = json_value_as_string(oe->value);
                    char *str = (char *)malloc((s->string_size + 1) * sizeof(*str));
                    strncpy(str, s->string, s->string_size);
                    str[s->string_size] = 0;
                    program[piPointer].value = makeString(str);
                }
                if (json_value_as_number(oe->value) != NULL)
                {
                    program[piPointer].value = makeNumber(atof(json_value_as_number(oe->value)->number));
                }
            }
            if (0 == strcmp(oe->name->string, "functionName"))
            {
                bind_function("nop", &i_nop);
                bind_function("plus", &i_plus);
                bind_function("+", &i_plus);
                bind_function("min", &i_min);
                bind_function("-", &i_min);
                bind_function("mul", &i_mul);
                bind_function("*", &i_mul);
                bind_function("pop", &i_pop);
                bind_function("stdout", &i_stdout);
                bind_function("concat", &i_concat);
                bind_function("rconcat", &i_rconcat);
                bind_function("charCode", &i_charCode);
                bind_function("randInt", &i_randInt);
                bind_function("eq", &i_eq);
                bind_function("and", &i_and);
                bind_function("dup", &i_dup);
                bind_function("gt", &i_gt);
                bind_function("lt", &i_lt);
                bind_function("not", &i_not);
                bind_function("or", &i_or);
                bind_function("ppc", &i_ppc);
                bind_function("stacksize", &i_stacksize);
                bind_function("jz", &i_jz);
                bind_function("jgz", &i_jgz);
                bind_function("{", &i_br_open);
                bind_function("}", &i_br_close);
                bind_function("pause", &i_pause);
                bind_function("exit", &i_exit);
                bind_function("goto", &i_goto);
                bind_function("setContext", &i_setContext);
                bind_function("getContext", &i_getContext);
                bind_function("hasContext", &i_hasContext);
                bind_function("delContext", &i_delContext);

                void *const element = hashmap_get(&foreignFunctions, json_value_as_string(oe->value)->string, strlen(json_value_as_string(oe->value)->string));
                if (element != NULL)
                {
                    program[piPointer].function_pointer = element;
                }
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
    ppc = ppc + 1;
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

void registerForeignFunction(char *name, void *func)
{
    hashmap_put(&foreignFunctions, name, strlen(name), func);
}

void pause()
{
    i_pause();
}

void resume()
{
    running = true;
}