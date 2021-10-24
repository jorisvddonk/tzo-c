#include <stdbool.h>
#include "thirdparty/hashmap.h"
#include "thirdparty/json.h"

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
extern struct hashmap_s labelmap;
extern struct hashmap_s context;
extern struct hashmap_s foreignFunctions;

void run();
void step();
void pause();
void resume();
struct json_value_s *loadFileGetJSON(char *filename);
void initRuntime();
void initLabelMapFromJSONObject(struct json_object_s *obj);
void initProgramListFromJSONArray(struct json_array_s *array);
Value *makeString(char *str);
Value *makeNumber(float val);
void registerForeignFunction(char *name, void *func);
void _push(Value val);
Value _pop();
Value _popS();
float asInt_f(Value val);
char *asString(Value val);

#endif /* TZO_H */