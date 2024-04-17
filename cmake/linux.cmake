set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS}  -Wno-void-pointer-to-enum-cast -Wno-deprecated-declarations")
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

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "/usr")
endif()

if(NOT CMAKE_LIBRARY_PATH)
    set(CMAKE_LIBRARY_PATH "${CMAKE_PREFIX_PATH}/lib")
endif()

if(TARGET_ARCH STREQUAL "arm64")
  set(ARCH_SUFFIX "-arm64")
elseif(TARGET_ARCH STREQUAL "x86")
  set(ARCH_SUFFIX "-x86")
  add_definitions(-DELF)
  set(BUILD_MMX ON)
endif()

if(NOT TARGET_ARCH MATCHES "${CMAKE_SYSTEM_PROCESSOR}")
  if(TARGET_ARCH MATCHES "arm64")
    if(NOT CMAKE_C_COMPILER)
      set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc-12")
    endif()
    if(NOT CMAKE_LIBRARY_PATH)
      set(CMAKE_LIBRARY_PATH "${CMAKE_PREFIX_PATH}/usr/lib/aarch64-linux-gnu")
    endif()
  elseif(TARGET_ARCH MATCHES "86")
    if(NOT CMAKE_C_COMPILER)
      set(CMAKE_C_COMPILER "i686-linux-gnu-gcc-12")
    endif()
    if(NOT CMAKE_LIBRARY_PATH)
      set(CMAKE_LIBRARY_PATH "${CMAKE_PREFIX_PATH}/lib/i386-linux-gnu")
    endif()
  endif()
endif()

add_definitions(-DLINUX)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CMAKE_PREFIX_PATH}/include
  ${CMAKE_PREFIX_PATH}/include/SDL2
)

string(REPLACE " " ";" library_paths ${CMAKE_LIBRARY_PATH})
foreach(lib_path ${library_paths})
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -L${lib_path}
  )
endforeach()

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND mkdir -p ../engine/releases/LINUX/Logs
  COMMAND mkdir -p ../engine/releases/LINUX/Paks
  COMMAND mkdir -p ../engine/releases/LINUX/Saves
  COMMAND mkdir -p ../engine/releases/LINUX/ScreenShots
  COMMAND cp -a ${PROJECT_NAME} ../engine/releases/LINUX/${PROJECT_NAME}${ARCH_SUFFIX}
)
