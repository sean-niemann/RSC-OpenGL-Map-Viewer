#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include <stdint.h>

void texture_load_dir(const char *dirname, uint32_t *textures, GLboolean transparent);
GLuint texture_load(const char *fname, GLboolean transparent);

#endif // TEXTURE_H_INCLUDED
