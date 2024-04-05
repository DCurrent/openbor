cmake_minimum_required(VERSION 3.28)

project(OpenBOR VERSION 4.0)

# Targets
option(BUILD_WIN    "Windows" OFF)
option(BUILD_LINUX  "Linux" OFF)
option(BUILD_DARWIN "MacOS" OFF)
option(BUILD_WII    "Nintendo Wii Console" OFF)

# Backends
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

# Global Configuration & Dependencies
set(SDKPATH "")
set(COMMON_COMPILER_FLAGS "-Wall -Wno-void-pointer-to-enum-cast -fsigned-char -std=gnu99")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(PNG REQUIRED)

# Derived Target Compiler Configuration
if(BUILD_LINUX)
    set(BUILD_SDL     ON)
	set(BUILD_OPENGL  ON)
	set(BUILD_LOADGL  ON)
	set(BUILD_GFX     ON)
	set(BUILD_VORBIS  ON)
	set(BUILD_WEBM    ON)
	set(BUILD_PTHREAD ON)	
endif()

if(BUILD_DARWIN)
    set(SDKPATH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
	set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -target arm64-apple-macos11 -isysroot ${SDKPATH}")
    set(BUILD_LINUX   ON)
    set(BUILD_SDL     ON)
	set(BUILD_GFX     ON)
	set(BUILD_VORBIS  ON)
	set(BUILD_WEBM    ON)
	set(BUILD_PTHREAD ON)
endif()

# Default Compilation Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-Werror -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Werror -O3 -DNDEBUG")

# Grab ALL Source Files
file(GLOB_RECURSE SRC_CORE "engine/source/*.c" "engine/source/*.h")
file(GLOB         SRC_OBOR "engine/*.c"        "engine/*.h")
file(GLOB         SRC_SDL  "engine/sdl/*.c"    "engine/sdl/*.h")
file(GLOB         SRC_WII  "engine/wii/*.c"    "engine/wii/*.h")

# Exclude Tests
list(FILTER SRC_CORE EXCLUDE REGEX ".*test*" )

# Exclude Optional Sources
if(NOT BUILD_GFX)
    list(FILTER SRC_CORE EXCLUDE REGEX "engine/source/gfxlib/.*" )
endif()

# Overall Target Source List
set(SRC_FILES
  ${SRC_OBOR}
  ${SRC_CORE}
)

# Include Optional Source Files
if(BUILD_SDL)
  set(SRC_FILES	${SRC_FILES} ${SRC_SDL})
endif()

if(BUILD_WII)
  set(SRC_FILES	${SRC_FILES} ${SRC_WII})
endif()

# Final Target Source List
add_executable(${PROJECT_NAME} ${SRC_FILES})

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

# Default Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  z
  m
  ${PNG_LIBRARIES}
)

# Target Configuration
if(BUILD_LINUX)
  add_definitions(-DLINUX)
endif()

if(BUILD_DARWIN)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(OPENGL_LIBRARY OpenGL)
  find_library(CABRON_LIBRARY Carbon)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  find_library(IOKIT_LIBRARY IOKit)
  add_definitions(-DDARWIN)
  target_include_directories(${PROJECT_NAME} PRIVATE 
    /opt/homebrew/include
	/opt/homebrew/include/SDL2
	${SDKPATH}/usr/include/malloc
  )
  target_link_directories(${PROJECT_NAME} PRIVATE
  	/opt/homebrew/lib
  )
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-syslibroot,${SDKPATH}
    ${COCOA_LIBRARY}
    ${OPENGL_LIBRARY}
    ${CABRON_LIBRARY}
    ${AUDIOUNIT_LIBRARY}
    ${IOKIT_LIBRARY}  
)
endif()

if(BUILD_SDL)
  find_package(SDL2 REQUIRED)
  add_definitions(-DSDL=1)
  target_include_directories(${PROJECT_NAME} PRIVATE
    engine/sdl
  )
  target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2
	SDL2_gfx
	SDL2main
  )
endif()

if(BUILD_OPENGL)
  add_definitions(-DOPENGL)
endif()

if(BUILD_LOADGL)
  add_definitions(-DLOADGL)
endif()

if(BUILD_VORBIS)
  add_definitions(-DOV_EXCLUDE_STATIC_CALLBACKS)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    vorbisfile
	vorbis
	ogg
  )
endif()

if(BUILD_TREMOR)
  find_package(TREMOR)
  add_definitions(-DTREMOR)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    vorbisidec
    ogg
  )
endif()

if(BUILD_WEBM)
  add_definitions(-DWEBM)
  target_link_libraries(${PROJECT_NAME} PRIVATE vpx)
endif()

if(BUILD_PTHREAD)
  target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()
