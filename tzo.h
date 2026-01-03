#include <stdbool.h>
#include "thirdparty/hashmap.h"
#include "thirdparty/json.h"

#ifndef TZO_H
#define TZO_H

#define TZO_MAX_STACK_SIZE 1000
#define bind_function(V, X, Y)                                 \
  if (0 == strcmp(json_value_as_string(oe->value)->string, X)) \
  {                                                            \
    V->program[piPointer].function_pointer = Y;                \
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

typedef struct TzoVM TzoVM;

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
  void (*function_pointer)(TzoVM *);
} TzoInstr;

struct TzoVM
{
  TzoInstr *program;
  Value *stack;
  int stackSize;
  int programSize;
  int ppc;
  bool running;
  bool exited;
  struct hashmap_s labelmap;
  struct hashmap_s context;
  struct hashmap_s foreignFunctions;
};

/////////////////////////////////////////////////

TzoVM *createTzoVM();
void run(TzoVM *vm);
void step(TzoVM *vm);
void pause(TzoVM *vm);
void resume(TzoVM *vm);
struct json_value_s *loadFileGetJSON(TzoVM *vm, char *filename);
void initRuntime(TzoVM *vm);
void initLabelMapFromJSONObject(TzoVM *vm, struct json_object_s *obj);
void initProgramListFromJSONArray(TzoVM *vm, struct json_array_s *array);
void registerForeignFunction(TzoVM *vm, char *name, void *func);
void _push(TzoVM *vm, Value val);
Value _top(TzoVM *vm);
Value _pop(TzoVM *vm);
Value _popS(TzoVM *vm);

Value *makeString(char *str);
Value *makeNumber(float val);
float asInt_f(Value val);
char *asString(Value val);

#endif /* TZO_H */