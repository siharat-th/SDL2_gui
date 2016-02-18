#!/bin/bash

emcc \
../src/main.cpp \
../../../SDL_gui/GUI_utils.cpp \
../../../SDL_gui/GUI_BasicWidgets.cpp \
../../../SDL_gui/GUI_MainWin.cpp \
../../../SDL_gui/GUI_TopWin.cpp \
../../../SDL_gui/GUI_WinBase.cpp \
../../../SDL_gui/jsFileUtils.cpp \
../../../SDL_gui/SDL_gui.cpp \
../../../libs/boost/lib/emscripten/libboost_filesystem.a \
../../../libs/boost/lib/emscripten/libboost_system.a \
-I../../../SDL_gui \
-I../../../libs/boost/include \
-O2 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s LEGACY_GL_EMULATION=1 \
-s SDL2_IMAGE_FORMATS='["png","bmp"]' \
-o sdl2.html \
--emrun \
-Wall -std=c++14 -Wno-absolute-value \
-Wl,--as-needed -Wl,--gc-sections \
-s DISABLE_EXCEPTION_CATCHING=0 \
--preload-file ../data@data

