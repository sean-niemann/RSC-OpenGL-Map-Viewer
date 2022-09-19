#!/bin/sh
cd "$(dirname "$0")"

# compile
clang -O3 -Wno-deprecated-declarations -o mapview -lglfw -framework OpenGL src/main.c src/util.c src/texture.c src/model.c &&

# run
./mapview