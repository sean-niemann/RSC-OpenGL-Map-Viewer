#include <dirent.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef EMSCRIPTEN
    #include <GLFW/glfw3.h>
#elif __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include "util.h"
#include "texture.h"

#ifdef EMSCRIPTEN
  #include "stb/stb_image.h"
#endif

void texture_load_dir(const char *dirname, uint32_t *textures, GLboolean transparent) {
    DIR *dp = opendir(dirname);

    if (!dp) {
        ABORT("cannot open directory: %s", dirname);
    }

    struct dirent *dir;

    while((dir = readdir(dp))) {
        /* ensure no system files are loaded */
        if(/*dir->d_type == DT_REG && */dir->d_name[0] != '.') {
            char *path = concat(dirname, dir->d_name);
            char temp[32];
            strcpy(temp, path);

            /* remove file extension */
            char *e = strrchr(temp, '.');
            size_t index = (int) (e - temp);
            temp[index] = '\0';

            /* extract the id from the file path */
            int id = atoi(basename(temp));

            textures[id] = texture_load(path, transparent);

            free(path);
        }
    }
    closedir(dp);
}

uint32_t texture_load(const char *fname, GLboolean transparent) {
    uint32_t texture;

    #ifdef EMSCRIPTEN
    int width, height, comp;
    unsigned char* image = stbi_load(fname, &width, &height, &comp, !transparent ? STBI_rgb : STBI_rgb_alpha);

    if(image == NULL) {
        fprintf(stderr, "stbi_load: Failed to load texture (%s)", fname);
        exit(EXIT_FAILURE);
    }

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    if(comp == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if(comp == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(image);

    return texture;

    #else

    FILE *fp = fopen(fname, "rb");
    unsigned char header[54];

    if(!fp) {
        ABORT("cannot open file: %s", fname);
    }

    if(fread(header, 1, 54, fp) != 54) {
        ABORT("invalid bitmap header: %s", fname);
    }

    if(header[0] != 'B' || header[1] != 'M') {
        ABORT("texture not in bitmap format: %s", fname);
    }

    size_t width  = *(int*) & (header[0x12]);
    size_t height = *(int*) & (header[0x16]);
    size_t size   = width * height * (transparent ? 4 : 3); /* 3 -> 24 bpp, 4 -> 32 bpp (alpha channel) */

    unsigned char *data[size];

    fread(data, 1, size, fp);
    fclose(fp);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, transparent ? GL_RGBA : GL_RGB, width, height,
        0, transparent ? GL_BGRA_EXT : GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

    return texture;
    #endif
}
