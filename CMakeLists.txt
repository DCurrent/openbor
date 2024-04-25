cmake_minimum_required(VERSION 3.22)

project(OpenBOR VERSION 4.0 LANGUAGES C)

# Targets
option(BUILD_WIN    "Windows" OFF)
option(BUILD_LINUX  "Linux" OFF)
option(BUILD_DARWIN "MacOS" OFF)
option(BUILD_WII    "Nintendo Wii Console" OFF)

# Backends
option(TARGET_ARCH "[X86,AMD64,ARM64,UNIVERSAL]" OFF)
option(USE_SDL     "Enable SDL" OFF)
option(USE_OPENGL  "Enable OpenGL" OFF)
option(USE_LOADGL  "Enable SDL w/OpenGL Support" OFF)
option(USE_PTHREAD "Enable Multi-Threading" OFF)
option(USE_WEBM    "Enable WebM" OFF)
option(USE_VORBIS  "Enable Vorbis" OFF)
option(USE_TREMOR  "Enable Tremor" OFF)
option(USE_GFX     "Enable Video Scaling" OFF)

# Linker Options
option(ENABLE_STATIC     "Enable Static Linking" OFF)
option(ENABLE_STATIC_SDL "Enable SDL Static Linking" OFF)

# Debug Options
option(PRINT_ALL_VARS "Display All CMAKE Variables" OFF)

# Standardize String Values
string(TOLOWER "${TARGET_ARCH}" TARGET_ARCH)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CMAKE_SYSTEM_PROCESSOR)

# Detect Target Architecture
if(NOT TARGET_ARCH)
  set(TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Default Compilation Flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(COMMON_COMPILER_FLAGS "-Wall -fsigned-char")

# Generate Version Header
add_custom_target(VersionInfo ALL
  COMMAND bash version.sh
  COMMENT "Generating engine/version.h"
  WORKING_DIRECTORY ../engine
)

# Collect Source Files
file(GLOB         SRC_OBOR   "engine/*.c"                "engine/*.h")
file(GLOB_RECURSE SRC_CORE   "engine/source/*.c"         "engine/source/*.h")
file(GLOB         SRC_GFX    "engine/source/gfxlib/*.c"  "engine/source/gfxlib/*.h")
file(GLOB_RECURSE SRC_WEBM   "engine/source/webmlib/*.c" "engine/source/webmlib/*.h")
file(GLOB         SRC_SDL    "engine/sdl/*.c" "engine/sdl/*.h")
file(GLOB         SRC_OPENGL "engine/sdl/opengl.*")
file(GLOB         SRC_LOADGL "engine/sdl/loadgl.*")

# Exclude GFX & Tests
list(FILTER SRC_CORE EXCLUDE REGEX ".*test*" )
list(FILTER SRC_CORE EXCLUDE REGEX "engine/source/gfxlib/.*" )
list(FILTER SRC_CORE EXCLUDE REGEX "engine/source/webmlib/.*" )
list(FILTER SRC_SDL EXCLUDE REGEX "engine/sdl/.*gl.*" )

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
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_DARWIN ON)
    message("Build target not specified, BUILD_DARWIN=ON")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BUILD_LINUX ON)
    message("Build target not specified, BUILD_LINUX=ON")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
         CMAKE_SYSTEM_NAME MATCHES "MINGW")
    set(BUILD_WIN ON)
    message("Build target not specified, BUILD_WIN=ON")
  else()
    message(FATAL_ERROR "Unsupported Host OS Detected!")    
  endif()
endif()

# Target Configuration Modules
if(BUILD_WIN)
  include("cmake/windows.cmake")
elseif(BUILD_DARWIN)
  include("cmake/macos.cmake")
elseif(BUILD_LINUX)
  include("cmake/linux.cmake")
elseif(BUILD_WII)
  include("cmake/wii.cmake")
endif()

# Include Optional Source Files
if(USE_GFX)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_GFX})
endif()

if(USE_SDL)
  add_definitions(-DSDL=1)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_SDL})
endif()

if(USE_WEBM)
  add_definitions(-DWEBM)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_WEBM})
endif()

if(USE_OPENGL)
  add_definitions(-DOPENGL)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_OPENGL})
endif()

if(USE_LOADGL)
  add_definitions(-DLOADGL)
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_LOADGL})
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
if(USE_SDL)
  target_include_directories(${PROJECT_NAME} PRIVATE
    engine/sdl
  )
  if(ENABLE_STATIC_SDL)
    target_link_libraries(${PROJECT_NAME} PUBLIC
      -Wl,-Bstatic
    )
  endif() 
  target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL2
    SDL2main
  )
endif()

# General Static Linkage
if(ENABLE_STATIC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bstatic
  )
endif()

if(USE_VORBIS)
  add_definitions(-DOV_EXCLUDE_STATIC_CALLBACKS)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    vorbisfile
    vorbis
    ogg
  )
endif()

if(USE_TREMOR)
  add_definitions(-DTREMOR)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    vorbisidec
    ogg
  )
endif()

if(USE_WEBM)
  target_link_libraries(${PROJECT_NAME} PUBLIC vpx)
endif()

if(USE_PTHREAD)
  target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
endif()

# Default Libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
  png
  z
)

# Disable Static Linkage for standard libraries
if(ENABLE_STATIC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bdynamic
  )
endif()

# Standard Libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
  m
)

# Target Finalization
if(BUILD_WIN)
  include("cmake/windows-finalize.cmake")
elseif(BUILD_DARWIN)
  include("cmake/macos-finalize.cmake")
endif()

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND cp -a ../engine/COMPILING ../engine/releases/COMPILING.txt
  COMMAND cp -a ../engine/LICENSE ../engine/releases/LICENSE.txt
  COMMAND cp -a ../engine/README ../engine/releases/README.txt
  COMMAND cp -a ../engine/translation.txt ../engine/releases/
)

# Debug CMake Variables
if(PRINT_ALL_VARS)
  get_cmake_property(variable_names VARIABLES)
  list(SORT variable_names)
  foreach (variable_name ${variable_names})
    message(STATUS "${variable_name}=${${variable_name}}")
  endforeach ()
endif()
