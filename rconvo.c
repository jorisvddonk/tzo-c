#include "raylib.h"
#include "tzo.h"
#include "json_ez.h"
#include "physfs.h"
#include <stdio.h>

#define MAX_TEXTURES 128

typedef struct
{
    int hotspot_x;
    int hotspot_y;
    unsigned width;
    unsigned height;
    Texture2D tex;
} ATexture;

typedef struct AMusic
{
    bool haveMusic;
    Music music;
} AMusic;

ATexture *textures;
int nextTexture = 0;

RenderTexture2D target;

void clearscr(){
    //
};

void drawFrame(TzoVM *vm)
{
    Value frame_index = _pop(vm); // number
    int n = frame_index.number_value;
    ATexture t = textures[n];
    DrawTexture(t.tex, 0 - t.hotspot_x, 0 - t.hotspot_y, RAYWHITE);
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

    if (!PHYSFS_exists(filename))
    {
        printf("error: file %s does not exist. aborting!\n", filename);
        exit(1);
    }

    PHYSFS_file *myfile = PHYSFS_openRead(filename);
    PHYSFS_sint64 file_size = PHYSFS_fileLength(myfile);
    char *buf = malloc(PHYSFS_fileLength(myfile) * sizeof(*buf));
    int length_read = PHYSFS_read(myfile, buf, 1, PHYSFS_fileLength(myfile));

    Image img = LoadImageFromMemory(".png", buf, length_read);
    Texture2D tex = LoadTextureFromImage(img);
    ATexture t = {(int)hotspot_x.number_value, (int)hotspot_y.number_value, tex.width, tex.height, tex};
    textures[nextTexture] = t;
    nextTexture++;
}

void beginDraw(TzoVM *vm)
{
    BeginTextureMode(target);
}

void endDraw(TzoVM *vm)
{
    EndTextureMode();
    Rectangle rec = {0, 0, target.texture.width, -target.texture.height};
    Rectangle recd = {0, 0, 3 * target.texture.width, 3 * target.texture.height};
    Vector2 vec = {0, 0};
    DrawTexturePro(target.texture, rec, recd, vec, 0.0, WHITE);
}

AMusic music = {false, NULL};

void playMusic(TzoVM *vm)
{
    char *f = asString(_top(vm));
    char filename[256];
    sprintf(filename, "%s", f);
    printf("loading %s", filename);
    PHYSFS_file *modfile = PHYSFS_openRead(filename);
    PHYSFS_sint64 file_size = PHYSFS_fileLength(modfile);
    char *buf = malloc(PHYSFS_fileLength(modfile) * sizeof(*buf));
    int length_read = PHYSFS_read(modfile, buf, 1, PHYSFS_fileLength(modfile));

    Music mus = LoadMusicStreamFromMemory(".mod", buf, length_read);
    music.music = mus;
    music.music.looping = true;
    music.haveMusic = true;
    PlayMusicStream(music.music);
}

int main(int argc, char *argv[])
{
    PHYSFS_init(argv[0]);
    InitWindow(800, 600, "Convo Test");
    InitAudioDevice();

    PHYSFS_mount("uqm-0.8.0-content.uqm", NULL, 0);

    TzoVM *vm = createTzoVM();
    TzoVM *questvm = createTzoVM();

    textures = malloc(MAX_TEXTURES * sizeof(*textures));

    target = LoadRenderTexture(320, 200);

    printf("Loading ");
    printf(argv[1]);
    struct json_value_s *root = loadFileGetJSON(vm, argv[1]);
    printf("File loaded!");
    struct json_object_s *rootObj = json_value_as_object(root);
    struct json_array_s *inputProgram = get_object_key_as_array(rootObj, "programList");
    struct json_object_s *labelMap = get_object_key_as_object(rootObj, "labelMap");
    printf("initing: runtime...");
    initRuntime(vm);
    printf(" foreign functions...");
    registerForeignFunction(vm, "drawFrame", &drawFrame);
    registerForeignFunction(vm, "beginDraw", &beginDraw);
    registerForeignFunction(vm, "endDraw", &endDraw);
    registerForeignFunction(vm, "loadImage", &loadImage);
    registerForeignFunction(vm, "_playMusic", &playMusic);
    printf(" labelmap...");
    if (labelMap != NULL)
    {
        initLabelMapFromJSONObject(vm, labelMap);
    }
    printf(" programlist...");
    initProgramListFromJSONArray(vm, inputProgram);
    printf(" ...done!");

    printf("running!");
    run(vm);
    printf("loaded stuff");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (music.haveMusic)
        {
            UpdateMusicStream(music.music);
        }
        BeginDrawing();

        ClearBackground(RAYWHITE);
        run(vm);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}