#!/bin/sh
cd "$(dirname "$0")"

# compile
emcc    src/main.c \
        src/model.c \
        src/texture.c \
        src/util.c \
        -O3 \
        -s LEGACY_GL_EMULATION=1 \
        -s GL_FFP_ONLY=1 \
        -s STB_IMAGE=1 \
        -s USE_GLFW=3 \
        -o ./web/index.js \
        --preload-file ./data/sectors \
        --preload-file ./data/textures/ground \
        --preload-file ./data/textures/wall \
        --preload-file ./data/textures/model \
        --preload-file ./data/models \
        --preload-file ./data/model_locs.csv
#&&
# run
#emrun ./web/index.html --browser chrome

#-s GL_UNSAFE_OPTS=1 \
#-std=c99 \
