/*
A test for a Questmark interpreter, using dos-like (https://github.com/mattiasgustavsson/dos-like) to render.

To compile:
clang -target x86_64-pc-windows-gnu -I<PATH_TO_DOS-LIKE>/source -g3 questmark.c tzo.c <PATH_TO_DOS-LIKE>/source/dos.c -lgdi32 -luser32 -lwinmm -o questmark.exe

NOTE: this has a dependency on a font file ("questmark.fnt"), which is NOT included! - you can copy "volter.fnt" from dos-like and rename it "questmark.fnt"
TODO: create my own font file and include it :)

Usage:
1. compile questmark document to questmark json (NOTE: you may have to run this from a different working directory, as npx may actually run questmark.exe instead of questmark from npm!):
  npx questmark --input https://raw.githubusercontent.com/jorisvddonk/asking-about-flowers/master/questmark_comm/starbase.md --output questmark.json --no-run
2. run it via the interpreter!
  questmark.exe ./questmark.json

*/

#include <stdlib.h>
#include <time.h>
#include "tzo.h"
#include "json_ez.h"
#include "dos.h" // https://github.com/mattiasgustavsson/dos-like
#include <assert.h>

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
struct hashmap_s responseMap;

#define TEXT_COLOR_GAMETEXT 48
#define TEXT_COLOR_OPTIONTEXT 54
#define TEXT_COLOR_HEADER 97

typedef struct
{
  int pc;
  char *response;
} Answer;

int text_x = 0;
int text_y = 0;

void drawtext(char *str)
{
  printf("%s (%i, %i)", str, text_x, text_y);
  outtextxy(text_x * 4, text_y * 12, str);
  int numnl = 0;
  int strl = strlen(str);
  for (int i = 0; i < strl; i++)
  {
    if (str[i] == '\n')
    {
      numnl += 1;
    }
  }
  text_y = text_y + numnl + 1;
  printf("--%i (%i, %i)\n", numnl, text_x, text_y);
}

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

void clearscr()
{
  clearscreen();
  text_x = 0;
  text_y = 0;
  setcolor(TEXT_COLOR_HEADER);
  drawtext("TZO - QUESTMARK");
  text_x = 0;
  text_y = 2;
};

int clearResponse(void *const context, struct hashmap_element_s *const e)
{
  return -1;
}

void emit()
{
  setcolor(TEXT_COLOR_GAMETEXT);
  char *str = asString(_pop());
  printf("%s ", str);
  drawtext(str);
}

void getresponse()
{
  pause();
  setcolor(TEXT_COLOR_OPTIONTEXT);
  int size = hashmap_num_entries(&responseMap);
  text_x = 0;
  text_y = 35 - size; // TODO: calculate based on total number of lines actually used for the *text rendering*
  for (int i = 1; i <= size; i++)
  {
    char *k = toString(i);
    Answer *ans = hashmap_get(&responseMap, k, strlen(k));
    if (ans != NULL)
    {
      text_x = 0;
      char tmp[512];
      sprintf(tmp, "%s - %s", k, ans->response);
      drawtext(tmp); // will increment text_y;
    }
  }

  while (!shuttingdown())
  {
    char key = *readchars();
    if (keystate(KEY_ESCAPE))
    {
      break;
    }
    if (key != NULL)
    {
      char *k = toStringC(key);
      Answer *ans = hashmap_get(&responseMap, k, strlen(k));
      if (ans != NULL)
      {
        clearscr();
        Value num = *makeNumber(ans->pc);
        _push(num);
        int *value = 0;
        if (0 != hashmap_iterate_pairs(&responseMap, clearResponse, &value))
        {
          printf("failed to deallocate hashmap entries!!\n");
        }
        resume();
        break;
      }
    }
  }

  char *key = toString(0);
}

void response()
{
  Value a = _pop(); // number
  Value b = _pop(); // string
  int pc = a.number_value;
  char *str = asString(b);
  Answer *ans = malloc(sizeof *ans);
  ans->pc = pc;
  ans->response = str;

  int v = hashmap_num_entries(&responseMap) + 1;
  char *key = toString(v);
  hashmap_put(&responseMap, key, strlen(key), ans);
}

int main(int argc, char *argv[])
{
  setvideomode(videomode_640x480);
  int font = installuserfont("questmark.fnt");
  settextstyle(font, 0, 0, 0);
  setcolor(144);

  srand(time(NULL));
  printf("Loading %s ...\n", argv[1]);
  struct json_value_s *root = loadFileGetJSON(argv[1]);
  printf("File loaded (%d)\n", root->type);
  struct json_object_s *rootObj = json_value_as_object(root);
  struct json_array_s *inputProgram = get_object_key_as_array(rootObj, "programList");
  struct json_object_s *labelMap = get_object_key_as_object(rootObj, "labelMap");
  initRuntime();
  if (0 != hashmap_create(32, &responseMap))
  {
    assert(("failed to create responseMap hashmap", 0));
  }
  registerForeignFunction("emit", &emit);
  registerForeignFunction("getResponse", &getresponse);
  registerForeignFunction("response", &response);
  if (labelMap != NULL)
  {
    initLabelMapFromJSONObject(labelMap);
  }
  printf("- %i", inputProgram->length);
  initProgramListFromJSONArray(inputProgram);

  clearscr();
  cursoff();

  /*
  for (int i = 0; i < 255; i++)
  {
    setcolor(i);
    rectangle(i * 2, 0, 2, 400);
  }
  */

  printf("running!\n");
  run();
  printf("done!\n");
  setcolor(TEXT_COLOR_HEADER);
  drawtext("-< PRESS ESCAPE TO EXIT >-");

  while (!shuttingdown())
  {
    char key = *readchars();
    if (keystate(KEY_ESCAPE))
    {
      break;
    }
  }

  return 0;
}
