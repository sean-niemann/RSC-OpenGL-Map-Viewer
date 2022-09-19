#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include "model.h"
#include "util.h"

#define WINDOW_TITLE    "OpenGL Map Viewer"
#define WINDOW_WIDTH    (1200*1.0)
#define WINDOW_HEIGHT   (650*1.0)
#define FULLSCREEN      false
#define DATA_DIR        "./data/"

#define MAX_NORTH       37
#define MAX_SOUTH       55
#define MAX_EAST        48
#define MAX_WEST        67
#define TOP_FLOOR       2
#define UNDERGROUND     3

#define START_SECTOR_X  55
#define START_SECTOR_Y  48
#define START_SECTOR_H  0

#define START_ANGLE_X   35
#define START_ANGLE_Y   25
#define START_ANGLE_Z  -33

#define SECTOR_SIZE     23040 /* file size in bytes */

#define FIELD_OF_VIEW   60
#define DRAW_DISTANCE   200

#define MODEL_DEF_COUNT 405
#define MODEL_DEF_SCALE 140.0F
#define MODEL_LOC_COUNT 26675
#define MODEL_LOC_FILE  (DATA_DIR "model_locs.csv")
#define MODEL_TEXTURES  false /* unfinished */

#define WALL_HEIGHT         100
#define DIAG_WALL_OFFSET    12000

struct Tile {
    uint8_t height;
    uint8_t color;
    uint8_t texture;
    uint8_t roof;
    uint8_t wall_east;
    uint8_t wall_north;
    uint16_t wall_diag;
};

typedef struct {
    struct Tile tiles[48*4][48*4]; /* 48x48 grid multiplied by 4 levels */
    struct Point3D curr;
    bool loaded;
} Area; Area area;

enum CropStyle {
    CROP_NONE,
    CROP_TOP_RIGHT,
    CROP_TOP_LEFT,
    CROP_BOTTOM_RIGHT,
    CROP_BOTTOM_LEFT
};

struct Quad {
    uint8_t a, b, c, d;
} quad;

void tile_draw(struct Tile *tile, struct Point3D *point);
void tile_draw_vertex(uint8_t type, struct Point3D *point);
void tile_draw_tex_quad(struct Quad *quad, uint32_t texture, struct Point3D *point);
void tile_draw_tex_crop(struct Tile *tile, struct Quad *quad, uint8_t start, struct Point3D *point);
enum CropStyle tile_get_crop(struct Point3D *point);

/* rendering options with their respective default values */
int option_tile_crop    = 1,
    option_show_terrain = 1,
    option_underground  = 1,
    option_multi_story  = 1,
    option_show_info    = 1,
    option_show_walls   = 1,
#ifndef EMSCRIPTEN
    option_wire_frame   = 1,
    option_auto_spin    = 0,
    option_show_models  = 1;
#else
    option_wire_frame   = 0,
    option_auto_spin    = 1,
    option_show_models  = 0;
#endif

struct ModelLoc {
    uint16_t x, y;
    uint8_t dir;
    uint8_t width, height;
    uint16_t id;
    char* name;
};

void gl_render(void);
void gl_setup(void);
void init_vars(void);
void draw_info(void);
void draw_axis_indicator(void);
void open_sector(struct Point3D *point);
void clean(void);

/* GLFW callbacks */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void error_callback(int error, const char* description);

float ground_colors[256][3];
uint32_t ground_textures[256];
uint32_t wall_textures[256];
uint32_t model_textures[256];
GLFWwindow* window;
float angle_x, angle_y, angle_z;
float y_off;
double mouse_x, mouse_y;
uint16_t num_models;
struct ModelLoc model_locs[MODEL_LOC_COUNT];
struct Model model_defs[MODEL_DEF_COUNT];
struct Model onscreen_models[48][48];
float tile_scale = 4;

#endif // MAIN_H_INCLUDED
