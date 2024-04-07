set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-deprecated-declarations -Wno-void-pointer-to-enum-cast")

set(BUILD_SDL     ON)
set(BUILD_SDL_IO  ON)
set(BUILD_OPENGL  ON)
set(BUILD_LOADGL  ON)
set(BUILD_GFX     ON)
set(BUILD_VORBIS  ON)
set(BUILD_WEBM    ON)
set(BUILD_PTHREAD ON)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(amd64)|(AMD64)")
  add_definitions(-DELF -DAMD64)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)")
  add_definitions(-DELF)
  set(BUILD_MMX ON)
endif()

add_definitions(-DLINUX)

target_include_directories(${PROJECT_NAME} PRIVATE 
  /usr/include
  /usr/include/SDL2
)
