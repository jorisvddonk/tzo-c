#include <stdlib.h>
#include "raylib.h"
#include "tzo.h"
#include "json_ez.h"
#include "physfs.h"
#include "questvm.h"

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

void playMod(TzoVM *vm)
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

void drawResponses()
{
    struct hashmap_s responseMap = getResponseMap();
    int size = hashmap_num_entries(&responseMap);
    int text_x = 0;
    int text_y = 500 - (size * (16 + 5)); // TODO: calculate based on total number of lines actually used for the *text rendering*
    for (int i = 1; i <= size; i++)
    {
        char *k = toString(i);
        Answer *ans = hashmap_get(&responseMap, k, strlen(k));
        if (ans != NULL)
        {
            text_x = 0;
            char tmp[512];
            sprintf(tmp, "%s - %s", k, ans->response);
            DrawText(tmp, text_x, text_y, 16, BLUE);
            text_y += 16;
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
    registerForeignFunction(vm, "_playMod", &playMod);
    printf(" labelmap...");
    if (labelMap != NULL)
    {
        initLabelMapFromJSONObject(vm, labelMap);
    }
    printf(" programlist...");
    initProgramListFromJSONArray(vm, inputProgram);
    printf(" ...done!");

    printf("Loading questvm ");
    initQuestVM();
    printf(argv[2]);
    struct json_value_s *root_q = loadFileGetJSON(questvm, argv[2]);
    printf("File loaded!");
    struct json_object_s_q *rootObj_q = json_value_as_object(root_q);
    struct json_array_s_q *inputProgram_q = get_object_key_as_array(rootObj_q, "programList");
    struct json_object_s_q *labelMap_q = get_object_key_as_object(rootObj_q, "labelMap");
    printf("initing: runtime...");
    initRuntime(questvm);
    printf(" foreign functions...");
    registerForeignFunction(questvm, "emit", &emit);
    registerForeignFunction(questvm, "getResponse", &getresponse);
    registerForeignFunction(questvm, "response", &response);
    registerForeignFunction(questvm, "_playMod", &playMod);
    printf(" labelmap...");
    if (labelMap_q != NULL)
    {
        initLabelMapFromJSONObject(questvm, labelMap_q);
    }
    printf(" programlist...");
    initProgramListFromJSONArray(questvm, inputProgram_q);
    printf(" ...done!");

    printf("running!");
    run(vm);
    printf("loaded stuff");
    run(questvm);
    printf("started questvm");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (music.haveMusic)
        {
            UpdateMusicStream(music.music);
        }

        struct hashmap_s responseMap = getResponseMap();
        int key = GetKeyPressed();
        if (key != 0 && !questvm->running)
        {
            char *k = toStringC(key);
            Answer *ans = hashmap_get(&responseMap, k, strlen(k));
            if (ans != NULL)
            {
                Value num = *makeNumber(ans->pc);
                _push(questvm, num);
                int *value = 0;
                clearResponseMap();
                resume(questvm);
                clearCollectedText();
            }
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);
        run(vm);
        if (questvm->running)
        {
            run(questvm);
        }
        drawResponses();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}