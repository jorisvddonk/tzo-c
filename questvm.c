#include "tzo.h"
#include "questvm.h"
#include <stdio.h>
#include <assert.h>

struct hashmap_s responseMap;
static char *collectedText;

char *toString(int i)
{
  size_t needed = snprintf(NULL, 0, "%i", i) + 1;
  char *key = malloc(needed);
  sprintf(key, "%i", i);
  return key;
}

char *toStringC(int i)
{
  size_t needed = snprintf(NULL, 0, "%c", i) + 1;
  char *key = malloc(needed);
  sprintf(key, "%c", i);
  return key;
}

void response(TzoVM *vm)
{
  Value a = _pop(vm); // number
  Value b = _pop(vm); // string
  int pc = a.number_value;
  char *str = asString(b);
  Answer *ans = malloc(sizeof *ans);
  ans->pc = pc;
  ans->response = str;

  int v = hashmap_num_entries(&responseMap) + 1;
  char *key = toString(v);
  hashmap_put(&responseMap, key, strlen(key), ans);
}

void emit(TzoVM *vm)
{
  char *str = asString(_pop(vm));
  printf("%s ", str);
  sprintf(collectedText, "%s%s", collectedText, str);
}

int clearResponse(void *const context, struct hashmap_element_s *const e)
{
  return -1;
}

void clearResponseMap()
{
  int *ptr;
  if (0 != hashmap_iterate_pairs(&responseMap, clearResponse, &ptr))
  {
    cputs("failed to deallocate hashmap entries!!\n");
  }
}

struct hashmap_s getResponseMap()
{
  return responseMap;
}

char *getCollectedText()
{
  return collectedText;
}

void initQuestVM()
{
  if (0 != hashmap_create(32, &responseMap))
  {
    assert(("failed to create responseMap hashmap", 0));
  }
  collectedText = malloc(1024 * 4 * sizeof(*collectedText));
  clearCollectedText();
}

void clearCollectedText()
{
  memset(collectedText, 0, 1024 * 4);
}