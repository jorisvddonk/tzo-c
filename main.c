#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "tzo.h"
#include "json_ez.h"

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