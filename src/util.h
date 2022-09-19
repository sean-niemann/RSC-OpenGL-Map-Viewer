#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdio.h>
#include <stdint.h>

struct Point3D {
    uint16_t x, y, z;
};

#define ABORT(fmt, ...) do { \
    fprintf(stderr, "Abort: [" __FILE__ ":%d] " fmt "\n", __LINE__, __VA_ARGS__); \
    exit(9); \
} while(0)

void* xmalloc(size_t size);
ssize_t readline(char **lineptr, size_t *n, FILE *stream); /* reimplementation of 'getline' for windows support */
struct Point3D coordinates_to_sector(uint16_t x, uint16_t y);
long file_length(FILE *fp);
char* concat(const char *s1, const char *s2);
char** split(char* str, const char c);

#endif // UTIL_H_INCLUDED
