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
LOCAL_SRC_FILES := lib/armeabi-v7a/libSDL2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := png 
LOCAL_SRC_FILES := lib/armeabi-v7a/libpng.a
LOCAL_EXPORT_LDLIBS := -lz
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ogg
LOCAL_SRC_FILES := lib/armeabi-v7a/libogg.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := vorbisidec
LOCAL_SRC_FILES := lib/armeabi-v7a/libvorbisidec.a
LOCAL_STATIC_LIBRARIES := ogg
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := vpx
LOCAL_SRC_FILES := lib/armeabi-v7a/libvpx.a
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(PREBUILT_STATIC_LIBRARY)

#openbor
include $(CLEAR_VARS)

LOCAL_MODULE    := openbor
LOCAL_CFLAGS    := -g -O2 -Wall -Werror -Wno-unused-result -fsigned-char -fno-ident -freorder-blocks
LOCAL_CFLAGS    += -DLINUX -DSDL=1 -DANDROID=1 -DTREMOR=1 -DWEBM=1
LOCAL_CPPFLAGS  := ${LOCAL_CFLAGS}

LOCAL_C_INCLUDES  :=  \
	$(LOCAL_PATH)/src/ \
	$(LOCAL_PATH)/include/ \
	$(LOCAL_PATH)/include/png \
	$(LOCAL_PATH)/include/sdl \
	$(LOCAL_PATH)/../../.. \
	$(LOCAL_PATH)/../../../sdl \
	$(LOCAL_PATH)/../../../resources \
	$(LOCAL_PATH)/../../../source \
	$(LOCAL_PATH)/../../../source/adpcmlib \
	$(LOCAL_PATH)/../../../source/gamelib \
	$(LOCAL_PATH)/../../../source/gfxlib \
	$(LOCAL_PATH)/../../../source/pnglib \
	$(LOCAL_PATH)/../../../source/preprocessorlib \
	$(LOCAL_PATH)/../../../source/ramlib \
	$(LOCAL_PATH)/../../../source/randlib \
	$(LOCAL_PATH)/../../../source/scriptlib \
	$(LOCAL_PATH)/../../../source/tracelib2 \
	$(LOCAL_PATH)/../../../source/webmlib \
	$(LOCAL_PATH)/../../../source/webmlib/halloc \
	$(LOCAL_PATH)/../../../source/webmlib/nestegg

		
LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/../../../sdl/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/adpcmlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/gamelib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/gfxlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/pnglib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/preprocessorlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/ramlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/randlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/scriptlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/webmlib/*.c) \
	$(wildcard $(LOCAL_PATH)/../../../source/webmlib/*/*.c)) \
	jniutils.cpp

LOCAL_SRC_FILES += SDL_android_main.cpp

LOCAL_LDLIBS := -ldl -lGLESv2 -llog

LOCAL_STATIC_LIBRARIES := png vorbisidec SDL2 vpx

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/cpufeatures)

