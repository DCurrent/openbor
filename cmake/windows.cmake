set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-deprecated-declarations -fstack-protector-all")

set(BUILD_SDL     ON)
set(BUILD_SDL_IO  ON)
set(BUILD_OPENGL  ON)
set(BUILD_LOADGL  ON)
set(BUILD_GFX     ON)
set(BUILD_VORBIS  ON)
set(BUILD_WEBM    ON)
set(BUILD_PTHREAD ON)
set(BUILD_STATIC  ON)

cmake_policy(SET CMP0135 NEW)
include(FetchContent)

if(DOCKER_ARCH MATCHES "(amd64)|(AMD64)")
  FetchContent_Declare(
    sdl2-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-SDL2-2.30.2-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(sdl2-mingw)

  FetchContent_Declare(
    sdl2_gfx-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-SDL2_gfx-1.0.4-2-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(sdl2_gfx-mingw)

  FetchContent_Declare(
    zlib-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-zlib-1.3.1-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(zlib-mingw)

  FetchContent_Declare(
    vorbis-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libvorbis-1.3.7-2-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(vorbis-mingw)

  FetchContent_Declare(
    ogg-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libogg-1.3.5-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(ogg-mingw)

  FetchContent_Declare(
    png-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libpng-1.6.43-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(png-mingw)

  FetchContent_Declare(
    vpx-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libvpx-1.14.0-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(vpx-mingw)

  set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
  add_definitions(-DELF -DAMD64)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${sdl2-mingw_SOURCE_DIR}/mingw64/include
    ${sdl2-mingw_SOURCE_DIR}/mingw64/include/SDL2
    ${sdl2_gfx-mingw_SOURCE_DIR}/mingw64/include/SDL2
    ${zlib-mingw_SOURCE_DIR}/mingw64/include
    ${vorbis-mingw_SOURCE_DIR}/mingw64/include
    ${ogg-mingw_SOURCE_DIR}/mingw64/include
    ${png-mingw_SOURCE_DIR}/mingw64/include
    ${vpx-mingw_SOURCE_DIR}/mingw64/include
  )
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-Bstatic
    -L/usr/x86_64-w64-mingw32/lib
    -L${sdl2-mingw_SOURCE_DIR}/mingw64/lib
    -L${sdl2_gfx-mingw_SOURCE_DIR}/mingw64/lib
    -L${zlib-mingw_SOURCE_DIR}/mingw64/lib
    -L${vorbis-mingw_SOURCE_DIR}/mingw64/lib
    -L${ogg-mingw_SOURCE_DIR}/mingw64/lib
    -L${png-mingw_SOURCE_DIR}/mingw64/lib
    -L${vpx-mingw_SOURCE_DIR}/mingw64/lib
  )
elseif(DOCKER_ARCH MATCHES "(x86)|(X86)")
  FetchContent_Declare(
    sdl2-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-SDL2-2.30.2-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(sdl2-mingw)

  FetchContent_Declare(
    sdl2_gfx-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-SDL2_gfx-1.0.4-2-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(sdl2_gfx-mingw)

  FetchContent_Declare(
    zlib-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-zlib-1.3.1-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(zlib-mingw)

  FetchContent_Declare(
    vorbis-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libvorbis-1.3.7-2-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(vorbis-mingw)

  FetchContent_Declare(
    ogg-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libogg-1.3.5-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(ogg-mingw)

  FetchContent_Declare(
    png-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libpng-1.6.43-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(png-mingw)

  FetchContent_Declare(
    vpx-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-libvpx-1.14.0-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(vpx-mingw)

  set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
  add_definitions(-DELF)
  set(BUILD_MMX ON)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${sdl2-mingw_SOURCE_DIR}/mingw32/include
    ${sdl2-mingw_SOURCE_DIR}/mingw32/include/SDL2
    ${sdl2_gfx-mingw_SOURCE_DIR}/mingw32/include/SDL2
    ${zlib-mingw_SOURCE_DIR}/mingw32/include
    ${vorbis-mingw_SOURCE_DIR}/mingw32/include
    ${ogg-mingw_SOURCE_DIR}/mingw32/include
    ${png-mingw_SOURCE_DIR}/mingw32/include
    ${vpx-mingw_SOURCE_DIR}/mingw32/include
  )
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-Bstatic
    -L/usr/i686-w64-mingw32/lib
    -L${sdl2-mingw_SOURCE_DIR}/mingw32/lib
    -L${sdl2_gfx-mingw_SOURCE_DIR}/mingw32/lib
    -L${zlib-mingw_SOURCE_DIR}/mingw32/lib
    -L${vorbis-mingw_SOURCE_DIR}/mingw32/lib
    -L${ogg-mingw_SOURCE_DIR}/mingw32/lib
    -L${png-mingw_SOURCE_DIR}/mingw32/lib
    -L${vpx-mingw_SOURCE_DIR}/mingw32/lib
  )
endif()

add_definitions(-DWIN)

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND mkdir -p ../engine/releases/WINDOWS/Logs
  COMMAND mkdir -p ../engine/releases/WINDOWS/Paks
  COMMAND mkdir -p ../engine/releases/WINDOWS/Saves
  COMMAND mkdir -p ../engine/releases/WINDOWS/ScreenShots
  COMMAND cp -a ${PROJECT_NAME}.exe ../engine/releases/WINDOWS/
)
