set(SDKPATH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -target arm64-apple-macos11 -isysroot ${SDKPATH}")

set(BUILD_LINUX   ON)
set(BUILD_SDL     ON)
set(BUILD_GFX     ON)
set(BUILD_VORBIS  ON)
set(BUILD_WEBM    ON)
set(BUILD_PTHREAD ON)

add_definitions(-DLINUX)
add_definitions(-DDARWIN)

find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)

find_library(COCOA_LIBRARY Cocoa)
find_library(OPENGL_LIBRARY OpenGL)
find_library(CABRON_LIBRARY Carbon)
find_library(AUDIOUNIT_LIBRARY AudioUnit)
find_library(IOKIT_LIBRARY IOKit)

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
