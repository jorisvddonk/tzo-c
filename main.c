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

void test()
{
  printf("test CALLED!\n");
}

int main()
{
  srand(time(NULL));
  struct json_value_s *root = loadFileGetJSON("test.json");
  initRuntime();
  printf("inited runtime!\n");
  registerForeignFunction("test", &test);
  printf("loading program!\n");
  initProgramListFromJSONArray(json_value_as_array(root));
  printf("running!\n");
  run();
  printf("done!\n");
  return (0);
}