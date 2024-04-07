set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-deprecated-declarations -Wno-void-pointer-to-enum-cast")

set(BUILD_SDL     ON)
set(BUILD_GFX     ON)
set(BUILD_VORBIS  ON)
set(BUILD_WEBM    ON)
set(BUILD_PTHREAD ON)

add_definitions(-DLINUX)

target_include_directories(${PROJECT_NAME} PRIVATE 
  /usr/include
  /usr/include/SDL2
)
