if(TARGET_ARCH MATCHES "${CMAKE_SYSTEM_PROCESSOR}")
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_PREFIX_PATH}/include/SDL2
  )
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-Bstatic
    ${CMAKE_PREFIX_PATH}/lib
  )
else()
  if(TARGET_ARCH MATCHES "arm64")
    message(FATAL_ERROR "Cross-Compiling MinGW ARM64 Not Supported")
  elseif(TARGET_ARCH MATCHES "64")
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    target_include_directories(${PROJECT_NAME} PRIVATE
      /opt/mingw64/include
      /opt/mingw64/include/SDL2
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
      -Wl,-Bstatic
      -L/usr/x86_64-w64-mingw32/lib
      -L/opt/mingw64/lib
    )
  elseif(TARGET_ARCH MATCHES "86")
    set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
    target_include_directories(${PROJECT_NAME} PRIVATE
      /opt/mingw32/include
      /opt/mingw32/include/SDL2
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
      -Wl,-Bstatic
      -L/usr/i686-w64-mingw32/lib
      -L/opt/mingw32/lib
    )
  endif()
endif()

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
