set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-stringop-truncation -Wno-maybe-uninitialized -Wno-unused-result -Wno-void-pointer-to-enum-cast -Wno-deprecated-declarations")
set(ARCH_SUFFIX "")

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "/usr")
endif()

if(TARGET_ARCH STREQUAL "arm64")
  set(ARCH_SUFFIX "-arm64")
endif()

if(NOT TARGET_ARCH MATCHES "${CMAKE_SYSTEM_PROCESSOR}")
  if(TARGET_ARCH MATCHES "arm64")
    if(NOT CMAKE_C_COMPILER)
      set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc-12")
    endif()
  endif()
endif()

add_definitions(-DLINUX)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CMAKE_PREFIX_PATH}/include
  ${CMAKE_PREFIX_PATH}/include/SDL2
)

if(CMAKE_LIBRARY_PATH)
  target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_LIBRARY_PATH}
  )
endif()

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/LINUX/Logs
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/LINUX/Paks
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/LINUX/Saves
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/LINUX/ScreenShots
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ../engine/releases/LINUX/${PROJECT_NAME}${ARCH_SUFFIX}
)
