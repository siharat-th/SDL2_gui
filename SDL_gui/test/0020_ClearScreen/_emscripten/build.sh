#!/bin/bash

emcc \
../src/main.cpp \
../../../SDL_gui/GUI_utils.cpp \
-I../../../SDL_gui \
-s USE_SDL=2 -s LEGACY_GL_EMULATION=1 \
-o sdl2.html \
--emrun \
-Wall -std=c++14 -Wno-absolute-value \
-Wl,--as-needed -Wl,--gc-sections \
-s DISABLE_EXCEPTION_CATCHING=0
