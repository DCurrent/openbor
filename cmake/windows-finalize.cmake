if((CMAKE_HOST_SYSTEM MATCHES "MINGW") OR (CMAKE_HOST_SYSTEM MATCHES "Windows"))
  set(CMAKE_RC_COMPILER "windres")
  if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "$ENV{MINGW_PREFIX}")
  endif()
  if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "$ENV{MINGW_CHOST}-gcc")
  endif()
  if(NOT CMAKE_LIBRARY_PATH)
    set(CMAKE_LIBRARY_PATH "${CMAKE_PREFIX_PATH}/lib")
  endif()
else()
  if(TARGET_ARCH MATCHES "arm64")
    message(FATAL_ERROR "Cross-Compiling MinGW ARM64 Not Supported")
  elseif(TARGET_ARCH MATCHES "64")
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
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
    set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
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

# Populate Executable Resource Attributes
set_target_properties(${PROJECT_NAME} PROPERTIES
  LINK_FLAGS ${PROJECT_SOURCE_DIR}/engine/resources/${PROJECT_NAME}.res
)
add_custom_command(TARGET ${PROJECT_NAME}
  PRE_LINK
  COMMAND ${CMAKE_RC_COMPILER} ${PROJECT_NAME}.rc -O coff -o ${PROJECT_NAME}.res
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/engine/resources
)

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WINDOWS/Logs
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WINDOWS/Paks
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WINDOWS/Saves
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WINDOWS/ScreenShots
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.exe ../engine/releases/WINDOWS/${PROJECT_NAME}${ARCH_SUFFIX}.exe
)
