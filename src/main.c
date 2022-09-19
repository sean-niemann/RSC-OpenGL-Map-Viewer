#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef EMSCRIPTEN
    #include "emscripten.h"
#elif __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include "main.h"
#include "model.h"
#include "texture.h"
#include "util.h"

#include "stb/stb_easy_font.h"

int main(void) {
    /* initialize GLFW */
    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(error_callback);

    /* create window with GL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, FULLSCREEN ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    /* make the window's context current for the calling thread */
    glfwMakeContextCurrent(window);

    gl_setup();

    init_vars();

    open_sector(&area.curr);

    /* GLFW callbacks */
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    #ifdef EMSCRIPTEN
        emscripten_set_main_loop(gl_render, 60, !glfwWindowShouldClose(window));
    #else

    while (!glfwWindowShouldClose(window)) {
        /* draw calls */
        gl_render();
        
        /* swap front and back buffers */
        glfwSwapBuffers(window);

        /* poll and process events */
        glfwPollEvents();
    }
    #endif

    /* clean up */
    atexit(clean);

    return EXIT_SUCCESS;
}

void clean(void) {
    glfwDestroyWindow(window);
    glfwTerminate();
    for (unsigned i = 0; i < MODEL_DEF_COUNT; i++) {
        model_cleanup(&model_defs[i]);
    }
}

void error_callback(int error, const char* description) {
    ABORT("GLFW Error %d: %s", error, description);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    /* tile picking */
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // todo: backburner until linux/emscripten tri picking discrepencies are solved
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (option_auto_spin) option_auto_spin = false;
        double delta_x = mouse_x - xpos;
        double delta_y = mouse_y - ypos;
        angle_x -= delta_x / 4;
        angle_y -= delta_y / 4;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    /* tile scaling */
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        if (yoffset > 0 && tile_scale < 24) {
            tile_scale += 0.1;
        }
        if (yoffset < 0 && tile_scale > -12) {
            tile_scale -= 0.1;
        }
    /* zoom */
    } else {
        if (yoffset > 0 && angle_z < 14) {
            angle_z += 1.21;
        }
        if (yoffset < 0 && angle_z > -156) {
            angle_z -= 1.21;
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    bool press = area.loaded && action == GLFW_PRESS;
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_UP:
            if (press && area.curr.y != MAX_NORTH) {
                open_sector(&((struct Point3D) { area.curr.x, --area.curr.y, area.curr.z }));
            }
            break;
        case GLFW_KEY_DOWN:
            if (press && area.curr.y != MAX_SOUTH) {
                open_sector(&((struct Point3D) { area.curr.x, ++area.curr.y, area.curr.z }));
            }
            break;
        case GLFW_KEY_LEFT:
            if (press && area.curr.x != MAX_WEST) {
                open_sector(&((struct Point3D) { ++area.curr.x, area.curr.y, area.curr.z }));
            }
            break;
        case GLFW_KEY_RIGHT:
            if (press && area.curr.x != MAX_EAST) {
                open_sector(&((struct Point3D) { --area.curr.x, area.curr.y, area.curr.z }));
            }
            break;
        /* togglable options */
        case GLFW_KEY_1:     if (press) option_wire_frame   ^=1; break;
        case GLFW_KEY_2:     if (press) option_show_info    ^=1; break;
        case GLFW_KEY_3:     if (press) option_tile_crop    ^=1; break;
        case GLFW_KEY_4:     if (press) option_show_terrain ^=1; break;
        case GLFW_KEY_5:     if (press) option_show_walls   ^=1; break;
        case GLFW_KEY_6:     if (press) option_show_models  ^=1; break;
        case GLFW_KEY_M:     if (press) option_multi_story  ^=1; break;
        case GLFW_KEY_U:     if (press) option_underground  ^=1; break;
        case GLFW_KEY_SPACE: if (press) option_auto_spin    ^=1; break;
    }
}

void gl_setup(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_NORMALIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
}

// todo: for emscripten, need to rearrange OpenGL texture calls
void model_draw(struct Model *model, struct Tile *tile, struct Point3D *point) {
    if(!option_show_models) return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(MODEL_TEXTURES) glEnable(GL_TEXTURE_2D);

    float angle;
    switch(model->dir) {
        case 0:  angle = 0;   break;
        case 1:  angle = 45;  break;
        case 2:  angle = 90;  break;
        case 3:  angle = 135; break;
        case 4:  angle = 180; break;
        case 5:  angle = 225; break;
        case 6:  angle = 270; break;
        case 7:  angle = 315; break;
        default: angle = 0;   break;
    }
    /* used to minorly adjust models to be in the middle of the appropriate tile */
    float factor = tile_scale / 7.0F;
    float x_off = factor;
    float z_off = factor;
    // todo: handle diagonals
    if(model->dir == 2 || model->dir == 6) {
        z_off += (model->width * factor) - factor;
        x_off += (model->height * factor) - factor;
    } else if(model->dir == 0 || model->dir == 4) {
        x_off += (model->width * factor) - factor;
        z_off += (model->height * factor) - factor;
    }

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(point->x - 24 + x_off, -(area.tiles[point->x][point->z].height / 255.0F * tile_scale), point->z - 24 + z_off);
    glScalef(1 / MODEL_DEF_SCALE, 1 / MODEL_DEF_SCALE, 1 / MODEL_DEF_SCALE);
    glRotatef(angle, 0, 1, 0);

    int triangle = 0;
    while (triangle < model->face_count) {
        /* non-textured faces */
        if (model->face_fill_front[triangle] < 0 || model->face_fill_back[triangle] < 0) {
            glEnable(GL_COLOR_MATERIAL);
            int packed_col = model->face_fill_front[triangle] < 0 ? model->face_fill_front[triangle] : model->face_fill_back[triangle];
            int red   = (~packed_col >> 10 & 31) * 8,
                green = (~packed_col >> 5  & 31) * 8,
                blue  = (~packed_col       & 31) * 8;
            glColor3ub(red, green, blue);
            glDisable(GL_COLOR_MATERIAL);
        /* textured faces */
        } else {
            if(MODEL_TEXTURES) {
                float alpha = 1.0f;
                glColor4f(1, 1, 1, alpha);
                if(model->face_fill_front[triangle] > 0){
                    glBindTexture(GL_TEXTURE_2D, model_textures[model->face_fill_front[triangle]]);
                } else {
                    glBindTexture(GL_TEXTURE_2D, model_textures[model->face_fill_back[triangle]]);
                }
            } else {
                break;
                //glColor4f(0.2, 0.2, 0.2, 0.2);
            }
        }

        switch(model->face_vertex_count[triangle]) {
            case 3:
                glBegin(GL_TRIANGLES);
                break;
            case 4:
                glBegin(GL_QUADS);
                break;
            default:
                #ifdef EMSCRIPTEN
                /* GL_POLYGON not supported by emscripten */
                glBegin(GL_QUADS);
                #else
                glBegin(GL_POLYGON);
                #endif
                break;
        }

        if (model->face_vertices[triangle] != NULL) {
            unsigned i = 0;
            while (i < model->face_vertex_count[triangle]) {
                int point_a = model->face_vertices[triangle][i];
                float modelX = model->vertices_x[point_a],
                      modelZ = model->vertices_y[point_a],
                      modelY = model->vertices_z[point_a];
                switch (i) {
                    case 0:  glTexCoord2f(1, 0); break;
                    case 1:  glTexCoord2f(0, 0); break;
                    case 2:  glTexCoord2f(0, 1); break;
                    default: glTexCoord2f(1, 1); break;
                }
                glVertex3f(modelX, modelZ, -modelY);
                i++;
            }
        }
        
        if(MODEL_TEXTURES) glBindTexture(GL_TEXTURE_2D, 0);
        triangle++;
        glEnd();
    }

    /* roate and scale models */
    glRotatef(-angle, 0, 1, 0);
    glScalef(MODEL_DEF_SCALE, MODEL_DEF_SCALE, MODEL_DEF_SCALE);
	if(MODEL_TEXTURES) glDisable(GL_TEXTURE_2D);
    // reset draw parameters
    float x = -(point->x - 24) - x_off,
          y = area.tiles[point->x][point->z].height / 255. * tile_scale,
          z = -(point->z - 24) - z_off;
    glTranslatef(x, y, z);
}

void init_vars(void) {
    area.curr = (struct Point3D) { START_SECTOR_X, START_SECTOR_Y, START_SECTOR_H };

    texture_load_dir(DATA_DIR "textures/ground/", ground_textures, false);
    texture_load_dir(DATA_DIR "textures/model/", model_textures, false);
    texture_load_dir(DATA_DIR "textures/wall/", wall_textures, true);

    /* model load routine */
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(MODEL_LOC_FILE, "r");
    if (fp == NULL) {
        ABORT("cannot open file: %s", MODEL_LOC_FILE);
    }

    unsigned n = 0;

    while ((read = readline(&line, &len, fp)) != EOF) {
        char** tokens;

        tokens = split(line, ',');

        if (tokens) {
            model_locs[n].x      = atoi(*(tokens + 0));
            model_locs[n].y      = atoi(*(tokens + 1));
            model_locs[n].dir    = atoi(*(tokens + 2));
            model_locs[n].width  = atoi(*(tokens + 3));
            model_locs[n].height = atoi(*(tokens + 4));
            model_locs[n].id     = atoi(*(tokens + 5));
            model_locs[n].name   =      *(tokens + 6);
            
            /* strip the trailing new line character */
            model_locs[n].name[strlen(model_locs[n].name) - 1] = '\0';
            
            if (!model_defs[model_locs[n].id].loaded) {
                char fname[64];
                snprintf(fname, sizeof(fname), DATA_DIR "models/%s.ob3", model_locs[n].name);
                FILE *fp_m = fopen(fname, "rb");

                if (!fp_m){
                    ABORT("cannot open file: %s", fname);
                }

                long len = file_length(fp_m);

                char *buf = (char*) xmalloc(len * sizeof(char));
                fread(buf, len, 1, fp_m);
                fclose(fp_m);

                model_load(&model_defs[model_locs[n].id], buf);

                free(buf);
            }

            n++;
            free(tokens);
        }
    }

    fclose(fp);
    if (line) {
        free(line);
    }

    angle_x = START_ANGLE_X;
    angle_y = START_ANGLE_Y;
    angle_z = START_ANGLE_Z;

    unsigned i;
    for(i = 0; i < 64; i++) {
        ground_colors[i][0] = (255 - i * 4) / 255.;
        ground_colors[i][1] = (255 - i * 1.75) / 255.;
        ground_colors[i][2] = (255 - i * 4) / 255.;
    }
    for(i = 0; i < 64; i++) {
        ground_colors[i+64][0] = (i * 3) / 255.;
        ground_colors[i+64][1] = 144 / 255.;
        ground_colors[i+64][2] = 0;
    }
    for(i = 0; i < 64; i++) {
        ground_colors[i+128][0] = (192 - i * 1.5) / 255.;
        ground_colors[i+128][1] = (144 - i * 1.5) / 255.;
        ground_colors[i+128][2] = 0;
    }
    for(i = 0; i < 64; i++) {
        ground_colors[i+192][0] = (96 - i * 1.5) / 255.;
        ground_colors[i+192][1] = (48 + i * 1.5) / 255.;
        ground_colors[i+192][2] = 0;
    }

}

void gl_render(void) {
    if (!area.loaded) return;

    /* update mouse positions to later compute mouse deltas */
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* keep angles between -360 and 360 */
    angle_x = fmodf(angle_x, 360);
    angle_y = fmodf(angle_y, 360);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FIELD_OF_VIEW, WINDOW_WIDTH / (float) WINDOW_HEIGHT, 0.1, DRAW_DISTANCE);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, angle_z);
    glRotatef(angle_y, 1, 0, 0);
    glRotatef(angle_x, 0, 1, 0);
    glRotatef(180, 0, 0, 1);

    glPushMatrix(); {
        if (area.loaded) {
            for (unsigned x = 0; x < 48; x++) {
                for (unsigned z = 0; z < 48; z++) {
                    /* full render of the current selected plane */
                    struct Tile tile = area.tiles[x][z];
                    struct Point3D point = (struct Point3D) { x, 0, z };

                    tile_draw(&tile, &point);

                    /* draw object models */
                    model_draw(&(onscreen_models[x][z]), &tile, &point);

                    if(area.curr.z == 0) {
                        if(option_multi_story) {
                            /* buildings with a 2nd floor */
                            struct Tile tile_level_1 = area.tiles[x+48][z+48];
                            struct Point3D point_level_1 = (struct Point3D) { x, 1, z };
                            tile_draw(&tile_level_1, &point_level_1);
                            /* buildings with a 3rd floor */
                            struct Tile tile_level_2 = area.tiles[x+96][z+96];
                            struct Point3D point_level_2 = (struct Point3D) { x, 2, z };
                            tile_draw(&tile_level_2, &point_level_2);
                        }
                        if(option_underground) {
                            /* draws the underground visible from the ground floor */
                            struct Tile tile_level_3 = area.tiles[x+144][z+144];
                            struct Point3D point_level_3 = (struct Point3D) { x, 3, z };
                            tile_draw(&tile_level_3, &point_level_3);
                        }
                    }
                }
            }
        }
    } glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, 0.01, DRAW_DISTANCE);

    if (option_show_info) draw_info();
    draw_axis_indicator();

    if (option_auto_spin) angle_x++;
}

void draw_info(void) {
    static double prev_time;
    static int frame_count;
    static char str_fps[16];

    /* compute FPS (only update every second) */
    double cur_time = glfwGetTime();
    frame_count++;
    if (cur_time - prev_time >= 1.0) {
        sprintf(str_fps, "FPS: %u", frame_count);
        frame_count = 0;
        prev_time = cur_time;
    }

    /* draw onscreen info */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(5, WINDOW_HEIGHT - 5, -5);
    glRotatef(180, 0, 1, 0);
    glRotatef(180, 0, 0, 1);

    unsigned x = 5;
    unsigned y = 5; 

    char str_renderer[64];
    sprintf(str_renderer, "Renderer: %s", (char*) glGetString(GL_RENDERER));
    gl_draw_string(x, y, str_renderer); y += 12;
    gl_draw_string(x, y, "GL Version: 1.1"); y += 12;
    gl_draw_string(x, y, strstr(str_fps, "F") ? str_fps : "FPS: Calculating.."); y += 12;
    gl_draw_string(x, y, "Perspective: Ortho"); y += 12;
    char str_camera_pos[64];
    sprintf(str_camera_pos, "Camera Pos: %.2f %.2f %.2f",
        angle_x > 180 ? angle_x - 359 : angle_x,
        angle_y > 180 ? angle_y - 359 : angle_y,
        angle_z);
    gl_draw_string(x, y, str_camera_pos); y += 12;
    char str_sector[32];
    sprintf(str_sector, "Sector: %u %u %u", area.curr.z, area.curr.x, area.curr.y);
    gl_draw_string(x, y, str_sector); y += 12;
    char str_model_cnt[32];
    sprintf(str_model_cnt, "Model Count: %u", num_models);
    gl_draw_string(x, y, str_model_cnt); y += 12;
}

void draw_axis_indicator(void) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(WINDOW_WIDTH - START_ANGLE_X, WINDOW_HEIGHT - START_ANGLE_Y, START_ANGLE_Z);
    glRotatef(angle_x, 0, 1, 0);
    glRotatef(angle_y, 1, 0, 0);
    glRotatef(180, 0, 0, 1);
    glLineWidth(4); /* not supported by emscripten (GL spec doesn't require it) */
    /* X */
    glColor3f(0, 0.8, 0);
    glBegin(GL_LINES); {
        glVertex3f(0, 0, 0);
        glVertex3f(0, 40, 0);
    } glEnd();
    /* Y */
    glColor3f(0.8, 0, 0);
    glBegin(GL_LINES); {
        glVertex3f(0, 0, 0);
        glVertex3f(40, 0, 0);
    } glEnd();
    /* Z */
    glColor3f(0, 0.8, 0.8);
    glBegin(GL_LINES); {
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, 40);
    } glEnd();
}

void load_sector(struct Point3D *point, uint8_t plane) {
    char fname[32];

    snprintf(fname, sizeof(fname), DATA_DIR "sectors/h%ux%uy%u", plane, point->x, point->y);

    FILE *fp = fopen(fname, "rb");

    if (!fp) {
        ABORT("cannot open file: %s", fname);
    }

    struct Tile tile;

    uint8_t buf[SECTOR_SIZE];
    fread(buf, sizeof(buf), 1, fp);

    size_t n = 0;
    for (unsigned x = 0; x < 48; x++) {
        for (unsigned z = 0; z < 48; z++) {
            tile.height      = plane == 3 ? 0 : buf[n]; n++; /* set height to 0 if underground */
            tile.color       = buf[n++];
            tile.texture     = buf[n++];
            tile.roof        = buf[n++];
            tile.wall_east   = buf[n++];
            tile.wall_north  = buf[n++];
            /* diagonal walls are 32 bits */
            tile.wall_diag   = (buf[n] << 24) | (buf[n+1] << 16) | (buf[n+2] << 8) | buf[n+3]; n+=4;
            area.tiles[x + (plane * 48)][z + (plane * 48)] = tile;
        }
    }
    fclose(fp);
}

void open_sector(struct Point3D *point) {
    struct Tile tile;

    area.loaded = false;

    char str_area[16];
    snprintf(str_area, sizeof(str_area), "h%ux%uy%u", point->z, point->x, point->y);

    char* app_title = concat(WINDOW_TITLE " - ", str_area);
    glfwSetWindowTitle(window, app_title);
    free(app_title);

    load_sector(point, point->z);
    /* only render other planes while on the ground floor */
    if(point->z == 0) {
        load_sector(point, 1);
        load_sector(point, 2);
        load_sector(point, 3);
    }

    /* unload onscreen models from previous sector */
    memset(onscreen_models, 0, sizeof(onscreen_models[0][0]) * 48 * 48);

    /* populate new sector models */
    num_models = 0;
    for (unsigned i = 0; i < MODEL_LOC_COUNT; i++) {
        struct ModelLoc loc = model_locs[i];
        struct Point3D p = coordinates_to_sector(loc.x, loc.y);
        if(point->x == p.x && point->y == p.y && point->z == p.z){
            onscreen_models[loc.x % 48][loc.y % 48]        = model_defs[loc.id];
            onscreen_models[loc.x % 48][loc.y % 48].dir    = loc.dir;
            onscreen_models[loc.x % 48][loc.y % 48].width  = loc.width;
            onscreen_models[loc.x % 48][loc.y % 48].height = loc.height;
            num_models++;
        }
    }

    area.loaded = true;
}

void tile_draw(struct Tile *tile, struct Point3D *point) {
    /* draw rooves (todo) */
    if(tile->roof > 0) {
        
    }

    /* draw walls */
    if (option_show_walls) {
        if (tile->wall_east == 17 || tile->wall_north == 17 || tile->wall_diag == 17) { // prevent rendering of invisible walls (only temporary until transparancy issue is fixed)
            goto no_invis;
        }
        
        if (tile->wall_east) { /*   __   */
            quad = (struct Quad) { 1, 2, 6, 5 };
            tile_draw_tex_quad(&quad, wall_textures[tile->wall_east], point);
        }

        if (tile->wall_north) { /*   |   */
            quad = (struct Quad) { 1, 4, 8, 5 };
            tile_draw_tex_quad(&quad, wall_textures[tile->wall_north], point);
        }

        if (tile->wall_diag && tile->wall_diag < DIAG_WALL_OFFSET) { /*   /   */
            quad = (struct Quad) { 1, 3, 7, 5 };
            tile_draw_tex_quad(&quad, wall_textures[tile->wall_diag], point);
        }

        if (tile->wall_diag > DIAG_WALL_OFFSET && tile->wall_diag < (DIAG_WALL_OFFSET * 2)) { /*   \   */
            quad = (struct Quad) { 9, 10, 11, 12 };
            tile_draw_tex_quad(&quad, wall_textures[tile->wall_diag % DIAG_WALL_OFFSET], point);
        }
    }
    no_invis:

    if (!option_show_terrain) return;

    if (tile->texture) {
        /* prevent rendering of the 'black void' texture (underground, stairs, ladders) */
        if(tile->texture == 8) return;

        switch(tile_get_crop(point)) {
            case CROP_TOP_RIGHT:
                quad = (struct Quad) { 3, 4, 1, 4 };
                tile_draw_tex_crop(tile, &quad, 2, point);
                break;
            case CROP_TOP_LEFT:
                quad = (struct Quad) { 2, 3, 3, 4};
                tile_draw_tex_crop(tile, &quad, 1, point);
                break;
            case CROP_BOTTOM_RIGHT:
                quad = (struct Quad) { 3, 4, 2, 3 };
                tile_draw_tex_crop(tile, &quad, 1, point);
                break;
            case CROP_BOTTOM_LEFT:
                quad = (struct Quad) { 1, 4, 3, 4 };
                tile_draw_tex_crop(tile, &quad, 2, point);
                break;
            default: /* no crop required (standard quad) */
                quad = (struct Quad) { 1, 2, 3, 4 };
                tile_draw_tex_quad(&quad, ground_textures[tile->texture], point);
                break;
        }
    } else {
        /* render non textured tiles on the ground floor and underground */
        if(point->y == 1 || point->y == 2) return;
        glColor3f(ground_colors[tile->color][0],
                  ground_colors[tile->color][1],
                  ground_colors[tile->color][2]);
        glBegin(GL_QUADS); {
            tile_draw_vertex(1, point);
            tile_draw_vertex(2, point);
            tile_draw_vertex(3, point);
            tile_draw_vertex(4, point);
        } glEnd();
    }

    /* render wireframe for ground floor */
    if(option_wire_frame && (point->y == 0 || point->y == 3)) {
        glColor4f(0, 0, 0, 1);
        glLineWidth(1);
        unsigned i = 2;
        /* offset to raise the wireframe slightly (on both sides) above the tiles to make it more visible */
        #ifdef EMSCRIPTEN
        y_off = -0.030f;
        i--; /* prevent rendering of the underside *iff* using emscripten (performance gain, since FFP draw calls are sluggish for it) */
        #else
        y_off = -0.015f;
        #endif
        while (i--) {
            glBegin(GL_LINES); {
                /* horizonal lines */
                tile_draw_vertex(1, point);
                tile_draw_vertex(2, point);
                tile_draw_vertex(3, point);
                tile_draw_vertex(4, point);
                /* vertical lines */
                tile_draw_vertex(1, point);
                tile_draw_vertex(4, point);
                tile_draw_vertex(2, point);
                tile_draw_vertex(3, point);
            } glEnd();
            y_off *= -1; /* flip the y offset, so that we can render the underside */
        }
        y_off = 0.0f; /* reset the y offset since wireframe rendering has finished */
    }
}

void tile_draw_tex_quad(struct Quad *quad, uint32_t texture, struct Point3D *point) {
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS); {
        glTexCoord2f(0, 1); tile_draw_vertex(quad->a, point);
        glTexCoord2f(0, 0); tile_draw_vertex(quad->b, point);
        glTexCoord2f(1, 0); tile_draw_vertex(quad->c, point);
        glTexCoord2f(1, 1); tile_draw_vertex(quad->d, point);
    } glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

/* draws edged tile-textures as tris instead of quads for smoother rivers, pathways, etc. */
void tile_draw_tex_crop(struct Tile *tile, struct Quad *quad, uint8_t start, struct Point3D *point) {
    /* begin texture triangle */
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, ground_textures[tile->texture]);
    glBegin(GL_TRIANGLES); {
        glTexCoord2f(0, 1); tile_draw_vertex(start, point);
        glTexCoord2f(0, 0); tile_draw_vertex(quad->a, point);
        glTexCoord2f(1, 0); tile_draw_vertex(quad->b, point);
    } glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    /* begin overlay triangle (ground floor only) */
    if(point->y == 1 || point->y == 2) return;
    glColor3f(
        ground_colors[tile->color][0],
        ground_colors[tile->color][1],
        ground_colors[tile->color][2]);
    glBegin(GL_TRIANGLES); {
        tile_draw_vertex(start, point);
        tile_draw_vertex(quad->c, point);
        tile_draw_vertex(quad->d, point);
    } glEnd();
}

/* macro used to calculate tile (and/or wall) height */
#define TILE_HEIGHT(x, z, wall)({\
    plane_height - \
    (area.tiles[x][z].height + \
    (wall ? WALL_HEIGHT : 0)) / \
    255. * tile_scale + y_off;})

void tile_draw_vertex(uint8_t type, struct Point3D *point) {
    /* calculate y offsets for given plane */
    float plane_height;
    switch(point->y) {
        /* first floor */
        case 1:  plane_height = -1.56; break;
        /* second floor */
        case 2:  plane_height = -3.12; break;
        /* underground */
        case 3:  plane_height =  12.0; break;
        /* ground floor */
        default: plane_height =   0.0; break;
    }

    uint8_t x = point->x;
    uint8_t z = point->z;

    /* used for underground height correction when viewing in multi-story mode */
    uint8_t hx = point->y == 3 ? point->x + (48*3) : point->x;
    uint8_t hz = point->y == 3 ? point->z + (48*3) : point->z;

    /* draw calls proper [1-4: terrain vertices] [5: terrain connector] [6-9: wall vertices] [10-12: wall connectors] */
    switch(type) {
        case 1:  glVertex3f(x-24, TILE_HEIGHT(hx, hz, 0), z-24);                                                 break;
        case 2:  glVertex3f(x-24, z<47 ? TILE_HEIGHT(hx, hz+1, 0) : TILE_HEIGHT(hx>0 ? hx-1 : hx, hz, 0), z-23); break;
        case 3:  glVertex3f(x-23, z<47 && x<47 ? TILE_HEIGHT(hx+1, hz+1, 0) : TILE_HEIGHT(hx, hz, 0), z-23);     break;
        case 4:  glVertex3f(x-23, x<47 ? TILE_HEIGHT(hx+1, hz, 0) : TILE_HEIGHT(hx, hz>0 ? hz-1 : hz, 0), z-24); break;
        case 5:  glVertex3f(x-24, TILE_HEIGHT(hx, hz, 1), z-24);                                                 break;
        case 6:  glVertex3f(x-24, z<47 ? TILE_HEIGHT(hx, hz+1, 1) : TILE_HEIGHT(hx>0 ? hx-1 : hx, hz, 1), z-23); break;
        case 7:  if (z<47 && x<47) glVertex3f(x-23, TILE_HEIGHT(hx, hz+1, 1), z-23);                             break;
        case 8:  glVertex3f(x-23, x<47 ? TILE_HEIGHT(hx+1, hz, 1) : TILE_HEIGHT(hx, hz>0 ? hz-1 : hz, 1), z-24); break;
        case 9:  glVertex3f(x-23, TILE_HEIGHT(hx+1, hz, 0), z-24);                                               break;
        case 10: glVertex3f(x-24, z<47 && x<47 ? TILE_HEIGHT(hx+1, hz+1, 0) : TILE_HEIGHT(hx, hz, 0), z-23);     break;
        case 11: glVertex3f(x-24, TILE_HEIGHT(hx, hz+1, 1), z-23);                                               break;
        case 12: glVertex3f(x-23, TILE_HEIGHT(hx+1, hz, 1), z-24);                                               break;
    }
}

enum CropStyle tile_get_crop(struct Point3D *point) {
    uint16_t x = point->x + (point->y * 48);
    uint16_t z = point->z + (point->y * 48);

    if (!area.tiles[x][z].texture || !option_tile_crop) {
        return CROP_NONE;
    }

    uint8_t crop = 0b0000;

    /* northern tile */
    if (point->z > 0  && area.tiles[x][z-1].texture) crop |= 0b1000;
    /* southern tile */
    if (point->z < 47 && area.tiles[x][z+1].texture) crop |= 0b0100;
    /* eastern tile */
    if (point->x > 0  && area.tiles[x-1][z].texture) crop |= 0b0010;
    /* western tile */
    if (point->x < 47 && area.tiles[x+1][z].texture) crop |= 0b0001;

    switch(crop) {
        case 0b0000:
        case 0b0001:
        case 0b0100:
        case 0b0101: return CROP_TOP_RIGHT;

        case 0b0110: return CROP_TOP_LEFT;

        case 0b1001: return CROP_BOTTOM_RIGHT;

        case 0b0010:
        case 0b1000:
        case 0b1010: return CROP_BOTTOM_LEFT;
        
        default:     return CROP_NONE;
    }

}