#include <stdbool.h>

#ifndef TZO_H
#define TZO_H

#define TZO_MAX_STACK_SIZE 1000
#define bind_function(X, Y)                                    \
  if (0 == strcmp(json_value_as_string(oe->value)->string, X)) \
  {                                                            \
    program[piPointer].function_pointer = Y;                   \
  }
#define throw(X) \
  printf(X);     \
  exit(-1);

typedef enum
{
  push_string = 0,
  push_number = 1,
  invoke_function = 2,
} InstructionType;

typedef enum
{
  String = 0,
  Number = 1,
} ValueType;

typedef struct
{
  ValueType type;
  char *string_value;
  float number_value;
} Value;

typedef struct
{
  InstructionType type;
  Value *value;
  void (*function_pointer)();
} TzoInstr;

/////////////////////////////////////////////////

extern TzoInstr *program;
extern Value *stack;
extern int stackSize;
extern int programSize;
extern int ppc;
extern bool running;
extern bool exited;

void run();
void step();
struct json_value_s *loadFileGetJSON(char *filename);
void initProgramListFromJSONArray(struct json_array_s *array);

#endif /* TZO_H */