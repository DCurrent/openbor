if(NOT TARGET_ARCH MATCHES "${CMAKE_SYSTEM_PROCESSOR}")
  if(TARGET_ARCH MATCHES "arm64")
    message(FATAL_ERROR "Cross-Compiling MinGW ARM64 Not Supported")
  elseif(TARGET_ARCH MATCHES "64")
    if(CMAKE_C_COMPILER MATCHES "/usr/bin/cc")
      set(CMAKE_C_COMPILER "x86_64-w64-mingw32-gcc")
    endif()
    if(NOT CMAKE_PREFIX_PATH)
      set(CMAKE_PREFIX_PATH "/opt/mingw64")
    endif()
    if(NOT CMAKE_LIBRARY_PATH)
      set(CMAKE_LIBRARY_PATH "/usr/x86_64-w64-mingw32/lib ${CMAKE_PREFIX_PATH}/lib")
    endif()
  elseif(TARGET_ARCH MATCHES "86")
    if(CMAKE_C_COMPILER MATCHES "/usr/bin/cc")
      set(CMAKE_C_COMPILER "i686-w64-mingw32-gcc")
    endif()
    if(NOT CMAKE_PREFIX_PATH)
      set(CMAKE_PREFIX_PATH "/opt/mingw32")
    endif()
    if(NOT CMAKE_LIBRARY_PATH)
      set(CMAKE_LIBRARY_PATH "/usr/i686-w64-mingw32/lib ${CMAKE_PREFIX_PATH}/lib")
    endif()
  endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_PREFIX_PATH}/include
  ${CMAKE_PREFIX_PATH}/include/SDL2
)

string(REPLACE " " ";" library_paths ${CMAKE_LIBRARY_PATH})
foreach(lib_path ${library_paths})
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-Bstatic
    -L${lib_path}
  )
endforeach()

target_link_libraries(${PROJECT_NAME} PUBLIC
  -lsetupapi -lhid -lpsapi -lopengl32 -lwinmm -lole32 -loleaut32 -luuid -limm32 -lversion -mwindows
)

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND mkdir -p ../engine/releases/WINDOWS/Logs
  COMMAND mkdir -p ../engine/releases/WINDOWS/Paks
  COMMAND mkdir -p ../engine/releases/WINDOWS/Saves
  COMMAND mkdir -p ../engine/releases/WINDOWS/ScreenShots
  COMMAND cp -a ${PROJECT_NAME}.exe ../engine/releases/WINDOWS/${PROJECT_NAME}${ARCH_SUFFIX}.exe
)
