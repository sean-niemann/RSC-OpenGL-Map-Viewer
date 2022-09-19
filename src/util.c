#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void* xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        ABORT("malloc: failed to allocate %zu bytes", size);
    }
    return ptr;
}

struct Point3D coordinates_to_sector(uint16_t x, uint16_t y) {
    unsigned sect_x = 0;
    unsigned sect_y = 0;
    unsigned sect_z = 0;
    if (y >= 0 && y <= 1007) {
        sect_z = 0;
    } else if (y >= 1007 && y <= 1007 + 943) {
        sect_z = 1;
        y -= 943;
    } else if (y >= 1008 + 943 && y <= 1007 + (943 * 2)) {
        sect_z = 2;
        y -= 943 * 2;
    } else {
        sect_z = 3;
        y -= 943 * 3;
    }
    sect_x = (x / 48) + 48;
    sect_y = (y / 48) + 37;
    return (struct Point3D) { sect_x, sect_y, sect_z };
}

long file_length(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    return len;
}

char* concat(const char *s1, const char *s2) {
    char *res = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(res, s1);
    strcat(res, s2);
    return res;
}

ssize_t readline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (!lineptr || !stream || !n) {
        return EOF;
    }

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return EOF;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return EOF;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return EOF;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

char** split(char* str, const char c) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = c;
    delim[1] = 0;

    /* count how many elements will be extracted */
    while (*tmp) {
        if (c == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* add space for trailing token */
    count += last_comma < (str + strlen(str) - 1);

    /* add space for terminating null string so caller knows where the list of returned strings ends */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(str, delim);
        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
