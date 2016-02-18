LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := $(SDL_ROOT)
GUI_PATH := $(SDL_GUI_ROOT)/SDL_gui

LOCAL_C_INCLUDES := $(SDL_PATH)/include $(GUI_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	main.cpp

LOCAL_SRC_FILES +=  $(GUI_PATH)/SDL_gui.cpp \
                    $(GUI_PATH)/GUI_WinBase.cpp \
                    $(GUI_PATH)/GUI_TopWin.cpp \
                    $(GUI_PATH)/GUI_MainWin.cpp \
                    $(GUI_PATH)/GUI_BasicWidgets.cpp \
                    $(GUI_PATH)/GUI_utils.cpp 

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_ttf
LOCAL_CFLAGS += -D__ANDROID__ -std=c++11
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

APP_STL := gnustl_static

include $(BUILD_SHARED_LIBRARY)
