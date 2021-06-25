cmake_minimum_required(VERSION 3.1)
#set(CMAKE_VERBOSE_MAKEFILE ON)

project(OpenBOR)
set(OPENBOR_VERSION "3.0.7141")

#####################
# OPTIONS
#####################
option(BUILD_GFX "Build with GFX support" ON)
option(BUILD_VORBIS "Build with BUILD_VORBIS support" ON)

##############
# COMMON
##############
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_GFX REQUIRED SDL2_gfx)
pkg_search_module(VPX REQUIRED vpx)
pkg_search_module(PNG REQUIRED libpng)
pkg_search_module(ZLIB REQUIRED zlib)

set(BOR_INC
        engine
        engine/sdl
        engine/source
        engine/source/adpcmlib
        engine/source/gamelib
        engine/source/openborscript
        engine/source/pnglib
        engine/source/preprocessorlib
        engine/source/ramlib
        engine/source/randlib
        engine/source/scriptlib
        engine/source/webmlib
        engine/source/webmlib/halloc
        engine/source/webmlib/nestegg
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIRS}
        ${VPX_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        )

file(GLOB BOR_SRC
        engine/*.c
        engine/sdl/*.c
        engine/source/*.c
        engine/source/adpcmlib/*.c
        engine/source/gamelib/*.c
        engine/source/openborscript/*.c
        engine/source/pnglib/*.c
        engine/source/preprocessorlib/*.c
        engine/source/ramlib/*.c
        engine/source/randlib/*.c
        engine/source/scriptlib/*.c
        engine/source/webmlib/*.c
        engine/source/webmlib/halloc/*.c
        engine/source/webmlib/nestegg/*.c
        )

set(BOR_CFLAGS -g -Wall -fsigned-char -fomit-frame-pointer -DSDL -DSDL2 -DWEBM -DOPENGL -DLOADGL -D__CMAKE__)
set(BOR_LDFLAGS ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARIES} ${VPX_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} m)

##############
# VERSION.SH
##############
ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/engine/version.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/engine
        COMMAND bash version.sh)
list(APPEND BOR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/engine/version.h)

#####################
# OPTIONS
#####################
if (BUILD_GFX)
    list(APPEND BOR_INC engine/source/gfxlib)
    file(GLOB GFX_SRC engine/source/gfxlib/*.c)
    list(APPEND BOR_SRC ${GFX_SRC})
endif ()
if (BUILD_VORBIS)
    pkg_search_module(VORBIS REQUIRED vorbis)
    pkg_search_module(VORBISFILE REQUIRED vorbisfile)
    pkg_search_module(OGG REQUIRED ogg)
    list(APPEND BOR_INC ${VORBIS_INCLUDE_DIRS} ${VORBISFILE_INCLUDE_DIRS} ${OGG_INCLUDE_DIRS})
    list(APPEND BOR_CFLAGS -DOV_EXCLUDE_STATIC_CALLBACKS)
    list(APPEND BOR_LDFLAGS ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES} ${OGG_LIBRARIES} m)
endif ()

#####################
# PLATORM SPECIFIC
#####################
if (NINTENDO_SWITCH)
    #####################
    # SWITCH PLATORM
    #####################
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND BOR_CFLAGS -DNXLINK)
    endif ()
    list(APPEND BOR_LDFLAGS stdc++)
else ()
    #####################
    # LINUX PLATORM
    #####################
    list(APPEND BOR_CFLAGS -D__LINUX__ -DLINUX)
endif ()

#####################
# OpenBOR executable
#####################
add_executable(${CMAKE_PROJECT_NAME} ${BOR_SRC})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${BOR_INC})
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${BOR_CFLAGS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${BOR_LDFLAGS})

#####################
# SWITCH target
#####################
if (NINTENDO_SWITCH)
    nx_generate_nacp(${CMAKE_PROJECT_NAME}.nacp
            NAME "${CMAKE_PROJECT_NAME}"
            AUTHOR "cpasjuste"
            VERSION "${OPENBOR_VERSION}"
            )
    nx_create_nro(${CMAKE_PROJECT_NAME}
            NACP ${CMAKE_PROJECT_NAME}.nacp
            ICON "${CMAKE_SOURCE_DIR}/engine/resources/switch_icon.jpg"
            )
    add_custom_target(${PROJECT_NAME}_switch_release
            DEPENDS ${PROJECT_NAME}.nro
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${OPENBOR_VERSION}_switch.zip
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND cd ${CMAKE_BINARY_DIR}/release && zip -r ../${PROJECT_NAME}-${OPENBOR_VERSION}_switch.zip ${PROJECT_NAME}
            )
endif ()
