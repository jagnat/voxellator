#!/bin/bash
clang++ ../src/sdl_vox.cpp -I/usr/include/SDL2 -I/usr/include/GL -std=c++11 -D_REENTRANT \
-g -L/usr/lib -lSDL2 -lGLU -lGL -ovox
