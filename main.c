#include <stdlib.h>
#include <time.h>
#include "tzo.h"
#include "json_ez.h"

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

void test(TzoVM *vm)
{
  printf("test CALLED!\n");
}

int main()
{
  srand(time(NULL));
  TzoVM *vm = createTzoVM();
  struct json_value_s *root = loadFileGetJSON(vm, "test.json");
  initRuntime(vm);
  printf("inited runtime!\n");
  registerForeignFunction(vm, "test", &test);
  printf("loading program!\n");
  initProgramListFromJSONArray(vm, json_value_as_array(root));
  printf("running!\n");
  run(vm);
  printf("done!\n");
  return (0);
}