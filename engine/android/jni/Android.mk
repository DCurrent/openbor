# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := SDL2
ifeq ($(TARGET_ARCH_ABI),armeabi) 
LOCAL_SRC_FILES := lib/armeabi/libSDL2.so
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a) 
LOCAL_SRC_FILES := lib/armeabi-v7a/libSDL2.so
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := z
ifeq ($(TARGET_ARCH_ABI),armeabi) 
LOCAL_SRC_FILES := lib/armeabi/libz.a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a) 
LOCAL_SRC_FILES := lib/armeabi-v7a/libz.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := png
ifeq ($(TARGET_ARCH_ABI),armeabi) 
LOCAL_SRC_FILES := lib/armeabi/libpng.a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a) 
LOCAL_SRC_FILES := lib/armeabi-v7a/libpng.a
endif
LOCAL_STATIC_LIBRARIES := z
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ogg
ifeq ($(TARGET_ARCH_ABI),armeabi) 
LOCAL_SRC_FILES := lib/armeabi/libogg.a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a) 
LOCAL_SRC_FILES := lib/armeabi-v7a/libogg.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := vorbis
ifeq ($(TARGET_ARCH_ABI),armeabi) 
LOCAL_SRC_FILES := lib/armeabi/libvorbis.a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a) 
LOCAL_SRC_FILES := lib/armeabi-v7a/libvorbis.a
endif
LOCAL_STATIC_LIBRARIES := ogg
include $(PREBUILT_STATIC_LIBRARY)

#openbor
include $(CLEAR_VARS)

LOCAL_MODULE    := openbor
LOCAL_CFLAGS    := -g -DLINUX -DSDL=1 -DANDROID=1 -DGLES=1 -DEGL=1 -DGL_GLEXT_PROTOTYPES #-DOPENGL=1
LOCAL_CPPFLAGS  := ${LOCAL_CFLAGS}

LOCAL_C_INCLUDES  :=  \
	$(LOCAL_PATH) \
    $(LOCAL_PATH)/include/ \
    $(LOCAL_PATH)/include/zlib \
    $(LOCAL_PATH)/include/png \
    $(LOCAL_PATH)/include/vorbis \
    $(LOCAL_PATH)/include/ogg \
    $(LOCAL_PATH)/include/sdl \
	$(LOCAL_PATH)/../.. \
	$(LOCAL_PATH)/../../sdl \
	$(LOCAL_PATH)/../../resources \
	$(LOCAL_PATH)/../../source \
	$(LOCAL_PATH)/../../source/adpcmlib \
	$(LOCAL_PATH)/../../source/gamelib \
	$(LOCAL_PATH)/../../source/gfxlib \
	$(LOCAL_PATH)/../../source/pnglib \
	$(LOCAL_PATH)/../../source/preprocessorlib \
	$(LOCAL_PATH)/../../source/ramlib \
	$(LOCAL_PATH)/../../source/randlib \
	$(LOCAL_PATH)/../../source/scriptlib \
	$(LOCAL_PATH)/../../source/tracelib2

		
LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/../../sdl/*.c) \
	$(wildcard $(LOCAL_PATH)/../../*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/adpcmlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/gamelib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/gfxlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/pnglib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/preprocessorlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/ramlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/randlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../source/scriptlib/*.c)) \
	SDL_android_main.cpp


LOCAL_LDLIBS    := -lGLESv1_CM

LOCAL_STATIC_LIBRARIES := png vorbis SDL2

include $(BUILD_SHARED_LIBRARY)

