/*
UQM comm screen animation example
To compile:
clang -target x86_64-pc-windows-gnu -I<PATH_TO_DOS-LIKE>/source -I<PATH_TO_LODEPNG> -g3 anim.c tzo.c <PATH_TO_DOS-LIKE>/source/dos.c <PATH_TO_LODEPNG>/lodepng.c -lgdi32 -luser32 -lwinmm -o anim.exe
*/

#include <stdlib.h>
#include <time.h>
#include "tzo.h"
#include "json_ez.h"
#include "dos.h" // https://github.com/mattiasgustavsson/dos-like
#include <assert.h>
#include "lodepng.h"

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

#define TEXT_COLOR_GAMETEXT 48
#define TEXT_COLOR_OPTIONTEXT 54
#define TEXT_COLOR_HEADER 97
#define MAX_TEXTURES 128

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

int text_x = 0;
int text_y = 0;

Texture *textures;
int nextTexture = 0;

Color *palette;
int nextslot = 1;
bool first = true;

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

void clearscr()
{
  clearscreen();
};

void drawFrame()
{
  Value frame_index = _pop(); // number
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
        printf("No more slots!\n");
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

void loadImage()
{
  char *f = asString(_pop());
  char filename[256];
  sprintf(filename, "./res/%s", f);
  Value hotspot_x = _pop(); // number
  Value hotspot_y = _pop(); // number
  unsigned error;
  unsigned char *image = 0;
  unsigned width, height;

  error = lodepng_decode32_file(&image, &width, &height, filename);
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

void beginDraw()
{
}

void endDraw()
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
  printf("%i, %i\n", blitscreen->width, blitscreen->height);
  blitscreen->buffer = malloc(blitscreen->height * blitscreen->width * sizeof(uint8_t));
}

int main(int argc, char *argv[])
{
  textures = malloc(MAX_TEXTURES * sizeof(*textures));
  palette = malloc(256 * sizeof(*palette));
  for (int i = 0; i < 256; i++)
  {
    Color a = {0, 0, 0};
    palette[i] = a;
  }
  setvideomode(videomode_320x240);
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
  printf("initing runtime\n");
  initRuntime();
  printf("initing foreign functions\n");
  registerForeignFunction("drawFrame", &drawFrame);
  registerForeignFunction("beginDraw", &beginDraw);
  registerForeignFunction("endDraw", &endDraw);
  registerForeignFunction("loadImage", &loadImage);
  printf("initing labelmap\n");
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

  // got all textures loaded now!
  // set up blit screen buffers
  blit_screen_0 = malloc(sizeof(*blit_screen_0));
  initBlitScreen(blit_screen_0);

  run();

  for (uint8_t i = (uint8_t)0; i <= (uint8_t)254; i++)
  {
    Color x = palette[i];
    setpal(i, x.r / 4, x.g / 4, x.b / 4);
  }

  for (int i = (int)0; i <= (int)254; i++)
  {
    int r, g, b;
    getpal(i, &r, &g, &b);
  }

  printf("done!\n");
  setcolor(TEXT_COLOR_HEADER);

  while (!shuttingdown())
  {
    waitvbl();
    char key = *readchars();
    if (keystate(KEY_ESCAPE))
    {
      break;
    }

    frame += 1;
    if (frame > 1)
    {
      clearscr();
      run();
      frame = 0;
    }
  }

  return 0;
}
