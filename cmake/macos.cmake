# Find Dependencies
find_program(BREW brew -v)

if(BREW)
  message(NOTICE "Native Homebrew installation detected")
  set(CMAKE_PREFIX_PATH "/opt/homebrew")
  set(CMAKE_LIBRARY_PATH "${CMAKE_PREFIX_PATH}/lib")
elseif(NOT CMAKE_PREFIX_PATH)
  message(WARNING "Homebrew not installed: https://brew.sh")
  message(FATAL_ERROR "CMAKE_PREFIX_PATH and CMAKE_LIBRARY_PATH are required.")
endif()

set(SDKPATH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS}  -Wno-void-pointer-to-enum-cast -Wno-int-conversion")

set(BUILD_LINUX ON)
set(USE_SDL     ON)
set(USE_OPENGL  ON)
set(USE_LOADGL  ON)
set(USE_GFX     ON)
set(USE_VORBIS  ON)
set(USE_WEBM    ON)
set(USE_PTHREAD ON)

add_definitions(-DLINUX)
add_definitions(-DDARWIN)

find_library(COCOA_LIBRARY Cocoa)
find_library(OPENGL_LIBRARY OpenGL)
find_library(CARBON_LIBRARY Carbon)
find_library(AUDIOUNIT_LIBRARY AudioUnit)
find_library(IOKIT_LIBRARY IOKit)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CMAKE_PREFIX_PATH}/include
  ${CMAKE_PREFIX_PATH}/include/SDL2
  ${SDKPATH}/usr/include/malloc
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
  OSX_ARCHITECTURES "arm64"
  LINK_DIRECTORIES ${CMAKE_LIBRARY_PATH}
  LINK_FLAGS -headerpad_max_install_names
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${COCOA_LIBRARY}
  ${OPENGL_LIBRARY}
  ${CARBON_LIBRARY}
  ${AUDIOUNIT_LIBRARY}
  ${IOKIT_LIBRARY}  
)
