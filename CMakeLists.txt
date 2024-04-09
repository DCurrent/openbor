cmake_minimum_required(VERSION 3.22)

project(OpenBOR VERSION 4.0)

# Targets
option(BUILD_WIN    "Windows" OFF)
option(BUILD_LINUX  "Linux" OFF)
option(BUILD_DARWIN "MacOS" OFF)
option(BUILD_WII    "Nintendo Wii Console" OFF)

# Backends
option(DOCKER_ARCH   "Docker Architecture")
option(BUILD_STATIC  "Enable Static Linking" OFF)
option(BUILD_SDL     "Enable SDL" OFF)
option(BUILD_OPENGL  "Enable OpenGL" OFF)
option(BUILD_LOADGL  "Enable SDL w/OpenGL Support" OFF)
option(BUILD_PTHREAD "Enable Multi-Threading" OFF)
option(BUILD_WEBM    "Enable WebM" OFF)
option(BUILD_VORBIS  "Enable Vorbis" OFF)
option(BUILD_TREMOR  "Enable Tremor" OFF)
option(BUILD_GFX     "Enable Video Scaling" OFF)
option(BUILD_MMX     "Enable MMX Instructions" OFF)

# Default Compilation Flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(COMMON_COMPILER_FLAGS "-Wall -Wno-void-pointer-to-enum-cast -fsigned-char")

# Generate Version Header
add_custom_target(VersionInfo ALL
  COMMAND bash version.sh
  COMMENT "Generating engine/version.h"
  WORKING_DIRECTORY ../engine
)

# Collect Source Files
file(GLOB         SRC_OBOR "engine/*.c"               "engine/*.h")
file(GLOB         SRC_SDL  "engine/sdl/*.c"           "engine/sdl/*.h")
file(GLOB         SRC_WII  "engine/wii/*.c"           "engine/wii/*.h")
file(GLOB_RECURSE SRC_CORE "engine/source/*.c"        "engine/source/*.h")
file(GLOB         SRC_GFX  "engine/source/gfxlib/*.c" "engine/source/gfxlib/*.h")
file(GLOB         SRC_MMX  "engine/source/gfxlib/*.asm")

# Exclude GFX & Tests
list(FILTER SRC_CORE EXCLUDE REGEX ".*test*" )
list(FILTER SRC_CORE EXCLUDE REGEX "engine/source/gfxlib/.*" )

# Target Sources
set(SRC_FILES
  ${SRC_OBOR}
  ${SRC_CORE}
)

# Target Executable
add_executable(${PROJECT_NAME} ${SRC_FILES})
add_dependencies(${PROJECT_NAME} VersionInfo)

# Auto-Detect
if(NOT (BUILD_WIN OR BUILD_LINUX OR BUILD_DARWIN OR BUILD_WII))
  if(APPLE)
    set(BUILD_DARWIN ON)
    message("Build target not specified, BUILD_DARWIN=ON")
  elseif(UNIX)
    set(BUILD_LINUX ON)
    message("Build target not specified, BUILD_LINUX=ON")
  else()
    set(BUILD_WIN ON)
    message("Build target not specified, BUILD_WIN=ON")
  endif()
endif()

# Target Configuration Modules
if(BUILD_DARWIN)
  include("cmake/osx-arm.cmake")
elseif(BUILD_LINUX)
  include("cmake/linux.cmake")
endif()

# Include Optional Source Files
if(BUILD_GFX)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_GFX})
endif()

if(BUILD_MMX)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_MMX})
endif()

if(BUILD_SDL)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_SDL})
endif()

if(BUILD_WII)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_WII})
endif()

# Compilation Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-Werror -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Werror -O3 -DNDEBUG")

# Default Includes
target_include_directories(${PROJECT_NAME} PRIVATE 
  engine/
  engine/source
  engine/source/adpcmlib
  engine/source/gamelib
  engine/source/gfxlib
  engine/source/openborscript
  engine/source/pnglib
  engine/source/preprocessorlib
  engine/source/ramlib
  engine/source/randlib
  engine/source/scriptlib
  engine/source/tracelib2
  engine/source/webmlib
)

# Include & Library Configuration
if(BUILD_SDL)
  add_definitions(-DSDL=1)
  target_include_directories(${PROJECT_NAME} PRIVATE
    engine/sdl
  )
  target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bdynamic
    SDL2
    SDL2_gfx
    SDL2main
  )
endif()

# Static linkage skips SDL
if(BUILD_STATIC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bstatic
  )
endif()

if(BUILD_VORBIS)
  add_definitions(-DOV_EXCLUDE_STATIC_CALLBACKS)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    vorbisfile
    vorbis
    ogg
  )
endif()

if(BUILD_TREMOR)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    vorbisidec
    ogg
  )
endif()

if(BUILD_WEBM)
  add_definitions(-DWEBM)
  target_link_libraries(${PROJECT_NAME} PUBLIC vpx)
endif()

if(BUILD_PTHREAD)
  target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
endif()

# Default Libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
  png
  z
)

# Static linkage disabled for default libraries
if(BUILD_STATIC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bdynamic
  )
endif()

# Standard Libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
  m
)

# Target Configuration Modules (Finalization) 
if(BUILD_DARWIN)
  include("cmake/osx-universal.cmake")
endif()

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND cp -a ../engine/COMPILING ../engine/releases/COMPILING.txt
  COMMAND cp -a ../engine/LICENSE ../engine/releases/LICENSE.txt
  COMMAND cp -a ../engine/README ../engine/releases/README.txt
  COMMAND cp -a ../engine/translation.txt ../engine/releases/
)
