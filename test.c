#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "tzo.h"
#include "json_ez.h"
#include <assert.h>

TzoInstr *program;
Value *stack;
int stackSize;
int programSize;
int ppc;
bool running;
bool exited;
struct hashmap_s labelmap;

int main(int argc, char *argv[])
{
  srand(time(NULL));
  printf("Loading %s ...\n", argv[1]);
  struct json_value_s *root = loadFileGetJSON(argv[1]);
  printf("File loaded (%d)\n", root->type);

  struct json_object_s *rootObj = json_value_as_object(root);
  struct json_array_s *inputProgram = get_object_key_as_array(rootObj, "input_program");
  struct json_object_s *expected = get_object_key_as_object(rootObj, "expected");
  struct json_array_s *expectedStack = get_object_key_as_array(expected, "stack");
  printf("-> size: %d\n", inputProgram->length);
  initProgramListFromJSONArray(inputProgram);
  run();
  printf("Done; running asserts!\n");

  Value *expectedStackV = malloc(expectedStack->length * sizeof expectedStack);
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

  printf(".\n");

  int expectedStackSize = i;
  assert(("Stack size must be equal", stackSize == expectedStackSize));
  for (i = 0; i < expectedStackSize; i++)
  {
    assert(("Type must be equal", stack[i].type == expectedStackV[i].type));
    if (stack[i].type == String)
    {
      assert(("String value must be equal", strcmp(stack[i].string_value, expectedStackV[i].string_value) == 0));
    }
    if (stack[i].type == Number)
    {
      assert(("Number value must be equal", stack[i].number_value == expectedStackV[i].number_value));
    }
  }

  printf("Done! Tests OK!\n");
  printf(".\n");
  return (0);
}