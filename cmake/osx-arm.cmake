set(SDKPATH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")

set(BUILD_LINUX   ON)
set(BUILD_SDL     ON)
set(BUILD_GFX     ON)
set(BUILD_VORBIS  ON)
set(BUILD_WEBM    ON)
set(BUILD_PTHREAD ON)

add_definitions(-DLINUX)
add_definitions(-DDARWIN)

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

set_target_properties(${PROJECT_NAME}
  PROPERTIES
  OSX_ARCHITECTURES "arm64"
  LINK_DIRECTORIES "/opt/homebrew/lib"
  LINK_FLAGS "-headerpad_max_install_names"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${COCOA_LIBRARY}
  ${OPENGL_LIBRARY}
  ${CABRON_LIBRARY}
  ${AUDIOUNIT_LIBRARY}
  ${IOKIT_LIBRARY}  
)
