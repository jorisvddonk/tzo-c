#include <stdlib.h>
#include <time.h>
#include "tzo.h"

TzoInstr *program;
Value *stack;
int stackSize;
int programSize;
int ppc;
bool running;
bool exited;
struct hashmap_s labelmap;
struct hashmap_s context;

int main()
{
  srand(time(NULL));
  struct json_value_s *root = loadFileGetJSON("test.json");
  initProgramListFromJSONArray(json_value_as_array(root));
  run();
  return (0);
}