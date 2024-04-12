if(TARGET_ARCH STREQUAL "amd64")
  target_include_directories(${PROJECT_NAME} PRIVATE
    /opt/mingw64/include
    /opt/mingw64/include/SDL2
  )
  target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-Bstatic
    -L/usr/x86_64-w64-mingw32/lib
    -L/opt/mingw64/lib
  )
elseif(TARGET_ARCH STREQUAL "x86")
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
