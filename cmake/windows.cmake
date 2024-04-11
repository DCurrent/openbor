set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-deprecated-declarations -fstack-protector-all")
set(ARCH_SUFFIX "")

set(BUILD_SDL     ON)
set(BUILD_SDL_IO  ON)
set(BUILD_OPENGL  ON)
set(BUILD_LOADGL  ON)
set(BUILD_GFX     ON)
set(BUILD_VORBIS  ON)
set(BUILD_WEBM    ON)
set(BUILD_PTHREAD ON)
set(BUILD_STATIC  ON)

add_definitions(-DWIN)

cmake_policy(SET CMP0135 NEW)
include(FetchContent)

if(DOCKER_ARCH MATCHES "(amd64)|(AMD64)")
  FetchContent_Declare(
    sdl2-mingw
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-SDL2-2.30.2-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(sdl2-mingw)

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

  set(ARCH_SUFFIX "-x64")
  set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
  add_definitions(-DELF -DAMD64)
elseif(DOCKER_ARCH MATCHES "(x86)|(X86)")
  FetchContent_Declare(
    sdl2-mingw
    URL https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-SDL2-2.30.2-1-any.pkg.tar.zst
  )
  FetchContent_MakeAvailable(sdl2-mingw)

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

  set(ARCH_SUFFIX "-x86")
  set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
  add_definitions(-DELF)
  set(BUILD_MMX ON)
else()
  message(NOTICE "Supported DOCKER_ARCH=[X86|AMD64]")
  message(FATAL_ERROR "Unsupported Docker Architecture")
endif()
