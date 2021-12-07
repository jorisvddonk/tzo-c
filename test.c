#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "tzo.h"
#include "json_ez.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  srand(time(NULL));
  printf("Loading %s ...\n", argv[1]);
  TzoVM *vm = createTzoVM();
  struct json_value_s *root = loadFileGetJSON(vm, argv[1]);
  printf("File loaded (%d)\n", root->type);

  struct json_object_s *rootObj = json_value_as_object(root);
  struct json_array_s *inputProgram = get_object_key_as_array(rootObj, "input_program");
  struct json_object_s *initialContext = get_object_key_as_object(rootObj, "initial_context");
  struct json_object_s *expected = get_object_key_as_object(rootObj, "expected");
  struct json_array_s *expectedStack = get_object_key_as_array(expected, "stack");
  struct json_object_s *expectedContext = get_object_key_as_object(expected, "context");
  printf("-> size: %d\n", inputProgram->length);
  initRuntime(vm);
  initProgramListFromJSONArray(vm, inputProgram);

  if (initialContext != NULL)
  {
    for (struct json_object_element_s *s = initialContext->start; s != NULL; s = s->next)
    {
      const char *key = s->name->string;
      if (json_value_as_string(s->value) != NULL)
      {
        hashmap_put(&vm->context, key, strlen(key), makeString(json_value_as_string(s->value)->string));
      }
      else if (json_value_as_number(s->value) != NULL)
      {
        struct json_number_s *n = json_value_as_number(s->value);
        float f = strtof(n->number, NULL);
        Value *num = makeNumber((float)f);
        hashmap_put(&vm->context, key, strlen(key), num);
      }
    }
  }

  run(vm);
  printf("Done; running asserts!\n");

  if (expectedStack != NULL)
  {

    Value *expectedStackV = malloc(expectedStack->length * sizeof(*expectedStackV));
    int i = 0;
    for (struct json_array_element_s *s = expectedStack->start; s != NULL; s = s->next)
    {
      if (s->value->type == json_type_string)
      {
        expectedStackV[i].type = String;
        expectedStackV[i].string_value = json_value_as_string(s->value)->string;
      }
      if (s->value->type == json_type_number)
      {
        expectedStackV[i].type = Number;
        expectedStackV[i].number_value = atof(json_value_as_number(s->value)->number);
      }
      i++;
    }

    int expectedStackSize = i;
    assert(("Stack size must be equal", vm->stackSize == expectedStackSize));
    for (i = 0; i < expectedStackSize; i++)
    {
      assert(("Type must be equal", vm->stack[i].type == expectedStackV[i].type));
      if (vm->stack[i].type == String)
      {
        assert(("String value must be equal", strcmp(vm->stack[i].string_value, expectedStackV[i].string_value) == 0));
      }
      if (vm->stack[i].type == Number)
      {
        assert(("Number value must be equal", vm->stack[i].number_value == expectedStackV[i].number_value)); // TODO: fix float comparison!
      }
    }
  }

  if (expectedContext != NULL)
  {
    for (struct json_object_element_s *s = expectedContext->start; s != NULL; s = s->next)
    {
      const char *key = s->name->string;
      Value *val = (Value *)hashmap_get(&vm->context, key, strlen(key));
      if (json_value_as_string(s->value) != NULL)
      {
        assert(("Type must be equal (String)", val->type == String));
        assert(("String value must be equal", strcmp(val->string_value, json_value_as_string(s->value)->string) == 0));
      }
      else if (json_value_as_number(s->value) != NULL)
      {
        assert(("Type must be equal (Number)", val->type == Number));
        assert(("Number value must be equal", val->number_value == strtof(json_value_as_number(s->value)->number, NULL))); // TODO: fix float comparison!
      }
    }
  }

  printf("Done! Tests OK!\n");
  printf(".\n");
  return (0);
}