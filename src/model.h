#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

struct Model {
    uint16_t vertex_count;
    int16_t *vertices_x;
    int16_t *vertices_y;
    int16_t *vertices_z;
    uint16_t face_count;
    uint8_t *face_vertex_count;
    int16_t **face_vertices;

    /* these can be 16 bits once the flag value for gourad shading is changed */
    int32_t *face_fill_back;
    int32_t *face_fill_front;
    int32_t *face_gouraud;

    uint8_t dir;
    uint8_t width, height;
    bool loaded;
};

void model_load(struct Model *model, char *data);
void model_cleanup(struct Model *model);
uint8_t get_ubyte(char byte);
uint16_t get_uint16(char *bytes, unsigned start);
int16_t get_int16(char *bytes, unsigned start);

#endif // MODEL_H_INCLUDED
