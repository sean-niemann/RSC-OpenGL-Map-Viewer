#include <stdlib.h>
#include "model.h"
#include "util.h"

const uint32_t USE_GOURAD_SHADING = 0xbc614e; /* 12345678 */

void model_load(struct Model *model, char *data) {
    unsigned offset = 0;

    uint16_t vert_count = get_uint16(data, offset);
    offset += 2;
    
    uint16_t face_count = get_uint16(data, offset);
    offset += 2;

    model->vertices_x =        (int16_t*)  xmalloc(vert_count * sizeof(int16_t));
    model->vertices_y =        (int16_t*)  xmalloc(vert_count * sizeof(int16_t));
    model->vertices_z =        (int16_t*)  xmalloc(vert_count * sizeof(int16_t));
    model->face_vertex_count = (uint8_t*)  xmalloc(face_count * sizeof(uint8_t));
    model->face_vertices =     (int16_t**) xmalloc(face_count * sizeof(int16_t*));
    model->face_fill_back =    (int32_t*)  xmalloc(face_count * sizeof(int32_t));
    model->face_fill_front =   (int32_t*)  xmalloc(face_count * sizeof(int32_t));
    model->face_gouraud =      (int32_t*)  xmalloc(face_count * sizeof(int32_t));

    unsigned i;

    for (i = 0; i < vert_count; i++) {
        model->vertices_x[i] = get_int16(data, offset);
        offset += 2;
    }

    for (i = 0; i < vert_count; i++) {
        model->vertices_y[i] = get_int16(data, offset);
        offset += 2;
    }

    for (i = 0; i < vert_count; i++) {
        model->vertices_z[i] = get_int16(data, offset);
        offset += 2;
    }

    model->vertex_count = vert_count;

    for (i = 0; i < face_count; i++) {
        model->face_vertex_count[i] = get_ubyte(data[offset++]);
    }

    for (i = 0; i < face_count; i++) {
        model->face_fill_back[i] = get_int16(data, offset);
        offset += 2;
        if (model->face_fill_back[i] == 0x7FFF) {
            model->face_fill_back[i] = USE_GOURAD_SHADING;
        }
    }

    for (i = 0; i < face_count; i++) {
        model->face_fill_front[i] = get_int16(data, offset);
        offset += 2;
        if (model->face_fill_front[i] == 0x7FFF) {
            model->face_fill_front[i] = USE_GOURAD_SHADING;
        }
    }

    for (i = 0; i < face_count; i++) {
        uint8_t n = get_ubyte(data[offset++]);
        model->face_gouraud[i] = n ? USE_GOURAD_SHADING : 0;
    }

    for (i = 0; i < face_count; i++) {
        model->face_vertices[i] = (int16_t*) xmalloc(model->face_vertex_count[i] * sizeof(int16_t));
        for (unsigned fv = 0; fv < model->face_vertex_count[i]; fv++) {
            if (vert_count <= 0xFF) {
                model->face_vertices[i][fv] = get_ubyte(data[offset]);
                offset++;
            } else {
                model->face_vertices[i][fv] = get_uint16(data, offset);
                offset += 2;
            }
        }
    }

    model->face_count = face_count;

    model->loaded = true;
}

void model_cleanup(struct Model *model) {
    free(model->vertices_x);
    free(model->vertices_y);
    free(model->vertices_z);
    free(model->face_vertex_count);

    for (unsigned i = 0; i < model->face_count; i++) {
        free(model->face_vertices[i]);
    }
    free(model->face_vertices);

    free(model->face_fill_back);
    free(model->face_fill_front);
    free(model->face_gouraud);
}

uint8_t get_ubyte(char byte) {
    return byte & 0xFF;
}

uint16_t get_uint16(char *bytes, unsigned start) {
    return (get_ubyte(bytes[start]) << 8) + get_ubyte(bytes[start + 1]);
}

int16_t get_int16(char *bytes, unsigned start) {
    int i = get_ubyte(bytes[start]) * 0x100 + get_ubyte(bytes[start + 1]);
    if (i > 0x7FFF) {
        i -= 0x10000;
    }
    return i;
}
