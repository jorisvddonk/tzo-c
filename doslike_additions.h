#include "dos.h" // https://github.com/mattiasgustavsson/dos-like

struct music_t *loadmod_frombuf(uint8_t *infile, size_t filesize);

#ifdef DOS_IMPLEMENTATION
struct music_t *loadmod_frombuf(uint8_t *infile, size_t filesize)
{
  uint8_t *data = (uint8_t *)malloc(filesize + sizeof(struct music_t) + sizeof(jar_mod_context_t));
  uint8_t *file = data + sizeof(struct music_t) + sizeof(jar_mod_context_t);
  memcpy(file, infile, filesize);
  if (!data)
    return NULL;
  struct music_t *music = (struct music_t *)data;
  music->format = MUSIC_FORMAT_MOD;
  jar_mod_context_t *modctx = (jar_mod_context_t *)(music + 1);
  if (!jar_mod_init(modctx) || !jar_mod_load(modctx, (void *)file, (int)filesize))
  {
    free(data);
    return NULL;
  }
  modctx->modfile = (muchar *)file;
  modctx->modfilesize = (int)filesize;
  return music;
}
#endif
