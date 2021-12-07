/*
UQM comm screen animation + quest text example
To compile (on Windows, via clang):
clang -target x86_64-pc-windows-gnu \
  -I<PATH_TO_DOS-LIKE>/source \
  -I<PATH_TO_LODEPNG> \
  -I<PATH_TO_LIBCURL>/include \
  -I<PATH_TO_PHYSFS>/src \
  -g3 \
  convo.c \
  tzo.c \
  <PATH_TO_DOS-LIKE>/source/dos.c \
  <PATH_TO_LODEPNG>/lodepng.c \
  <PATH_TO_LIBCURL>/bin/libcurl-x64.dll \
  <PATH_TO_PHYSFS>/build/libphysfs.dll \
  -lgdi32 -luser32 -lwinmm \
  -o convo.exe
  
To run:
1. make sure you have required .dll files available here
2. ./convo.exe ./yehat.json ./yehat_speech.json

(the tool will automatically download required .uqm content file if not available)
*/

#include <stdlib.h>
#include <time.h>
#include "tzo.h"
#include "json_ez.h"
#include "dos.h" // https://github.com/mattiasgustavsson/dos-like
#include <assert.h>
#include "lodepng.h"
#include <curl/curl.h>
#include "physfs.h"

#define TEXT_COLOR_GAMETEXT 1
#define TEXT_COLOR_OPTIONTEXT 2
#define TEXT_COLOR_HEADER 3
#define PALETTE_START 4
#define MAX_TEXTURES 128

struct hashmap_s responseMap;
int text_x = 0;
int text_y = 0;

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}
typedef struct
{
  int pc;
  char *response;
} Answer;

typedef struct
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Color;

typedef struct
{
  int hotspot_x;
  int hotspot_y;
  unsigned width;
  unsigned height;
  unsigned char *textureRef;
} Texture;

typedef struct
{
  uint8_t *buffer;
  int width;
  int height;
} Screenlike;

Texture *textures;
int nextTexture = 0;

Color *palette;
int nextslot = 4;
bool first = true;

char *collectedText;
int collectedTextPtr = 0;
int lastStroke = 0;

int frame = 0;

Screenlike *blit_screen_0;

void m_blit(int x, int y, unsigned char *source, int width, int height, int srcx, int srcy, int srcw, int srch, int colorkey, Screenlike *blit_tgt)
{
  uint8_t *dst = blit_tgt->buffer + x + y * blit_tgt->width;
  uint8_t *src = source + srcx + srcy * width;
  for (int iy = 0; iy < srch; ++iy)
  {
    for (int ix = 0; ix < srcw; ++ix)
    {
      if (src[ix] != colorkey)
      {
        dst[ix] = src[ix];
      }
    }
    src += width;
    dst += blit_tgt->width;
  }
}

void drawtext(char *str)
{
  //cputs("%s (%i, %i)", str, text_x, text_y);
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
  //cputs("--%i (%i, %i)\n", numnl, text_x, text_y);
}

void clearscr()
{
  clearscreen();
  text_x = 0;
  text_y = 10;
  setcolor(TEXT_COLOR_HEADER);
};

char *toString(int i)
{
  size_t needed = sprintf(NULL, 0, "%i", i) + 1;
  char *key = malloc(needed);
  sprintf(key, "%i", i);
  return key;
}
char *toStringC(int i)
{
  size_t needed = sprintf(NULL, 0, "%c", i) + 1;
  char *key = malloc(needed);
  sprintf(key, "%c", i);
  return key;
}

void drawFrame(TzoVM *vm)
{
  Value frame_index = _pop(vm); // number
  int n = frame_index.number_value;
  Texture t = textures[n];
  m_blit(0 - t.hotspot_x, 0 - t.hotspot_y, t.textureRef, t.width, t.height, 0, 0, t.width, t.height, -1, blit_screen_0);
}

void rgba_to_palette(char *rgba, char *r, const int count)
{
  int maxi = 0;
  for (int o = 0; o < count; ++o)
  {
    int _r, _g, _b;
    bool found = false;
    uint8_t f = 0;
    uint8_t cr = (uint8_t)rgba[0];
    uint8_t cg = (uint8_t)rgba[1];
    uint8_t cb = (uint8_t)rgba[2];
    for (uint8_t i = (uint8_t)0; i <= (uint8_t)254; i++)
    {
      if (!found)
      {
        Color p = palette[i];
        if ((p.r - cr) == 0 && (p.g - cg) == 0 && (p.b - cb) == 0 && (cr - p.r) == 0 && (cg - p.g) == 0 && (cb - p.b) == 0)
        {
          found = true;
          f = (uint8_t)i;
        }
      }
    }
    if (!found)
    {
      if (nextslot < 256)
      {
        Color z = {cr, cg, cb};
        palette[nextslot] = z;
        f = nextslot;
        nextslot += 1;
      }
      else
      {
        cputs("No more slots!\n");
      }
    }
    if (maxi < f)
    {
      maxi = f;
    }
    rgba += 4;
    r[0] = f;
    r += 1;
    found = false;
  }
}

void loadImage(TzoVM *vm)
{
  char *f = asString(_pop(vm));
  char filename[256];
  sprintf(filename, "%s", f);
  Value hotspot_x = _pop(vm); // number
  Value hotspot_y = _pop(vm); // number
  unsigned error;
  unsigned char *image = 0;
  unsigned width, height;

  if (!PHYSFS_exists(filename))
  {
    printf("error: file %s does not exist. aborting!\n", filename);
    exit(1);
  }

  PHYSFS_file *myfile = PHYSFS_openRead(filename);
  PHYSFS_sint64 file_size = PHYSFS_fileLength(myfile);
  char *buf = malloc(PHYSFS_fileLength(myfile) * sizeof(*buf));
  int length_read = PHYSFS_read(myfile, buf, 1, PHYSFS_fileLength(myfile));
  error = lodepng_decode_memory(&image, &width, &height, buf, length_read, LCT_RGBA, 8);
  if (error)
  {
    printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  else
  {
    char *img = malloc((width * height) * sizeof(*img));
    rgba_to_palette(image, img, (width * height));
    Texture t = {(int)hotspot_x.number_value, (int)hotspot_y.number_value, width, height, img};
    textures[nextTexture] = t;
    nextTexture++;
  }
}

void beginDraw(TzoVM *vm)
{
}

void endDraw(TzoVM *vm)
{
  maskblit(0, 0, blit_screen_0->buffer, blit_screen_0->width, blit_screen_0->height, 0, 0, blit_screen_0->width, blit_screen_0->height, -1);
}

void initBlitScreen(Screenlike *blitscreen)
{
  blitscreen->width = 0;
  blitscreen->height = 0;
  for (int i = 0; i < MAX_TEXTURES; i++)
  {
    Texture t = textures[i];
    // TODO: check hotspot math here. Doesn't matter for UQM graphics in general, though, as image-000 for comm screens is always encompassing the entire canvas.
    if (t.width - t.hotspot_x > blitscreen->width)
    {
      blitscreen->width = t.width - t.hotspot_x;
    }
    if (t.height - t.hotspot_y > blitscreen->height)
    {
      blitscreen->height = t.height - t.hotspot_y;
    }
  }
  blitscreen->buffer = malloc(blitscreen->height * blitscreen->width * sizeof(uint8_t));
}

void playMod(TzoVM *vm)
{
  Value a = vm->stack[vm->stackSize - 1]; // string
  char *filename = asString(a);
  printf("play mod %s\n", filename);
  if (!PHYSFS_exists(filename))
  {
    printf("error: mod file %s does not exist. aborting!\n", filename);
    exit(1);
  }
  PHYSFS_file *myfile = PHYSFS_openRead(filename);
  PHYSFS_sint64 file_size = PHYSFS_fileLength(myfile);
  char *buf = malloc(PHYSFS_fileLength(myfile) * sizeof(*buf));
  int length_read = PHYSFS_read(myfile, buf, 1, PHYSFS_fileLength(myfile));
  printf("loading...\n");
  struct music_t *music = loadmod_frombuf(buf, file_size);
  printf("loaded!\n");
  playmusic(music, 1, 255);
  printf("playing!\n");
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

void drawResponses()
{
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
}

void getresponse(TzoVM *vm)
{
  pause(vm);
  drawResponses();
}

int main(int argc, char *argv[])
{
  PHYSFS_init(argv[0]);
  TzoVM *vm = createTzoVM();
  TzoVM *questvm = createTzoVM();

  if (0 != hashmap_create(32, &responseMap))
  {
    assert(("failed to create responseMap hashmap", 0));
  }
  collectedText = malloc(1024 * 4 * sizeof(*collectedText));
  memset(collectedText, 0, 1024 * 4);

  textures = malloc(MAX_TEXTURES * sizeof(*textures));
  palette = malloc(256 * sizeof(*palette));
  for (int i = 0; i < 256; i++)
  {
    Color a = {0, 0, 0};
    palette[i] = a;
  }

  setvideomode(videomode_640x480);
  setpal(TEXT_COLOR_GAMETEXT, 100 / 4, 255 / 4, 30 / 4);
  setpal(TEXT_COLOR_OPTIONTEXT, 0 / 4, 255 / 4, 255 / 4);
  setpal(TEXT_COLOR_HEADER, 255 / 4, 255 / 4, 255 / 4);

  setcolor(144);
  cursoff();

  srand(time(NULL));
  cputs("Loading ");
  cputs(argv[1]);
  gotoxy(0, wherey() + 1);
  struct json_value_s *root = loadFileGetJSON(vm, argv[1]);
  cputs("File loaded!");
  gotoxy(0, wherey() + 1);
  struct json_object_s *rootObj = json_value_as_object(root);
  struct json_array_s *inputProgram = get_object_key_as_array(rootObj, "programList");
  struct json_object_s *labelMap = get_object_key_as_object(rootObj, "labelMap");
  cputs("initing: runtime...");
  initRuntime(vm);
  cputs(" foreign functions...");
  registerForeignFunction(vm, "drawFrame", &drawFrame);
  registerForeignFunction(vm, "beginDraw", &beginDraw);
  registerForeignFunction(vm, "endDraw", &endDraw);
  registerForeignFunction(vm, "loadImage", &loadImage);
  cputs(" labelmap...");
  if (labelMap != NULL)
  {
    initLabelMapFromJSONObject(vm, labelMap);
  }
  cputs(" programlist...");
  initProgramListFromJSONArray(vm, inputProgram);
  cputs(" ...done!");
  gotoxy(0, wherey() + 1);

  ///

  cputs("Loading ");
  cputs(argv[2]);
  gotoxy(0, wherey() + 1);
  struct json_value_s *root_q = loadFileGetJSON(questvm, argv[2]);
  cputs("File loaded!");
  gotoxy(0, wherey() + 1);
  struct json_object_s_q *rootObj_q = json_value_as_object(root_q);
  struct json_array_s_q *inputProgram_q = get_object_key_as_array(rootObj_q, "programList");
  struct json_object_s_q *labelMap_q = get_object_key_as_object(rootObj_q, "labelMap");
  cputs("initing: runtime...");
  initRuntime(questvm);
  cputs(" foreign functions...");
  registerForeignFunction(questvm, "emit", &emit);
  registerForeignFunction(questvm, "getResponse", &getresponse);
  registerForeignFunction(questvm, "response", &response);
  registerForeignFunction(questvm, "_playMod", &playMod);
  cputs(" labelmap...");
  if (labelMap_q != NULL)
  {
    initLabelMapFromJSONObject(questvm, labelMap_q);
  }
  cputs(" programlist...");
  initProgramListFromJSONArray(questvm, inputProgram_q);
  cputs(" ...done!");
  gotoxy(0, wherey() + 1);

  /*
  for (int i = 0; i < 255; i++)
  {
    setcolor(i);
    rectangle(i * 2, 0, 2, 400);
  }
  */

  const char *fname = "uqm-0.8.0-content.uqm";
  FILE *checkFile;
  if ((checkFile = fopen(fname, "r")) == NULL)
  {
    CURL *curl = curl_easy_init();
    if (curl)
    {
      CURLcode res;
      curl_easy_setopt(curl, CURLOPT_URL, "http://ftp.fau.de/gentoo/distfiles/a5/uqm-0.8.0-content.uqm");
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
      cputs("downloading UQM content file...");
      gotoxy(0, wherey() + 1);

      FILE *contentfile;
      contentfile = fopen(fname, "wb");
      if (contentfile)
      {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, contentfile);
        curl_easy_perform(curl);
        fclose(contentfile);
      }
      curl_easy_cleanup(curl);
    }
    else
    {
      cputs("CURL not loaded!? ERROR!");
      gotoxy(0, wherey() + 1);
    }
  }
  else
  {
    cputs("UQM content file found!");
    gotoxy(0, wherey() + 1);
    fclose(checkFile);
  }

  cputs("Mounting .uqm file(s)...");
  gotoxy(0, wherey() + 1);
  PHYSFS_mount("uqm-0.8.0-content.uqm", NULL, 0);
  PHYSFS_mount("P6014-0.2.1-prv-content.uqm", NULL, 0);

  cputs("Mounted! Starting!");
  gotoxy(0, wherey() + 1);
  clearscr();

  cputs("running!");
  gotoxy(0, wherey() + 1);
  run(vm);

  // got all textures loaded now!
  // set up blit screen buffers
  blit_screen_0 = malloc(sizeof(*blit_screen_0));
  initBlitScreen(blit_screen_0);

  run(vm);

  for (uint8_t i = (uint8_t)PALETTE_START; i <= (uint8_t)254; i++)
  {
    Color x = palette[i];
    setpal(i, x.r / 4, x.g / 4, x.b / 4);
  }

  for (int i = (int)0; i <= (int)254; i++)
  {
    int r, g, b;
    getpal(i, &r, &g, &b);
  }

  cputs("done!\n");
  setcolor(TEXT_COLOR_HEADER);

  run(questvm);

  while (!shuttingdown())
  {
    waitvbl();
    char key = *readchars();
    if (keystate(KEY_ESCAPE))
    {
      break;
    }
    if (key != NULL && !questvm->running && lastStroke == 0)
    {
      char *k = toStringC(key);
      Answer *ans = hashmap_get(&responseMap, k, strlen(k));
      if (ans != NULL)
      {
        Value num = *makeNumber(ans->pc);
        _push(questvm, num);
        int *value = 0;
        if (0 != hashmap_iterate_pairs(&responseMap, clearResponse, &value))
        {
          cputs("failed to deallocate hashmap entries!!\n");
        }
        resume(questvm);
        lastStroke = 10;
        memset(collectedText, 0, 1024 * 4);
      }
    }

    if (lastStroke > 0)
    {
      lastStroke -= 1;
    }

    frame += 1;
    if (frame > 1)
    {
      clearscr();
      setcolor(TEXT_COLOR_GAMETEXT);
      drawtext(collectedText);
      drawResponses();
      run(vm);
      if (questvm->running)
      {
        run(questvm);
      }
      frame = 0;
    }
  }

  return 0;
}
