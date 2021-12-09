#include "tzo.h"

typedef struct
{
  int pc;
  char *response;
} Answer;

void response(TzoVM *vm);
void emit(TzoVM *vm);
int clearResponse(void *const context, struct hashmap_element_s *const e);
struct hashmap_s getResponseMap();
void initQuestVM();
char *getCollectedText();
void clearCollectedText();
char *toString(int i);