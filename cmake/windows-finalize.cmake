  if(DOCKER_ARCH MATCHES "(amd64)|(AMD64)")
    target_include_directories(${PROJECT_NAME} PRIVATE
      ${sdl2-mingw_SOURCE_DIR}/mingw64/include
      ${sdl2-mingw_SOURCE_DIR}/mingw64/include/SDL2
      ${zlib-mingw_SOURCE_DIR}/mingw64/include
      ${vorbis-mingw_SOURCE_DIR}/mingw64/include
      ${ogg-mingw_SOURCE_DIR}/mingw64/include
      ${png-mingw_SOURCE_DIR}/mingw64/include
      ${vpx-mingw_SOURCE_DIR}/mingw64/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
      -Wl,-Bstatic
      -L/usr/x86_64-w64-mingw32/lib
      -L${sdl2-mingw_SOURCE_DIR}/mingw64/lib
      -L${zlib-mingw_SOURCE_DIR}/mingw64/lib
      -L${vorbis-mingw_SOURCE_DIR}/mingw64/lib
      -L${ogg-mingw_SOURCE_DIR}/mingw64/lib
      -L${png-mingw_SOURCE_DIR}/mingw64/lib
      -L${vpx-mingw_SOURCE_DIR}/mingw64/lib
    )
  elseif(DOCKER_ARCH MATCHES "(x86)|(X86)")
    target_include_directories(${PROJECT_NAME} PRIVATE
      ${sdl2-mingw_SOURCE_DIR}/mingw32/include
      ${sdl2-mingw_SOURCE_DIR}/mingw32/include/SDL2
      ${zlib-mingw_SOURCE_DIR}/mingw32/include
      ${vorbis-mingw_SOURCE_DIR}/mingw32/include
      ${ogg-mingw_SOURCE_DIR}/mingw32/include
      ${png-mingw_SOURCE_DIR}/mingw32/include
      ${vpx-mingw_SOURCE_DIR}/mingw32/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
      -Wl,-Bstatic
      -L/usr/i686-w64-mingw32/lib
      -L${sdl2-mingw_SOURCE_DIR}/mingw32/lib
      -L${zlib-mingw_SOURCE_DIR}/mingw32/lib
      -L${vorbis-mingw_SOURCE_DIR}/mingw32/lib
      -L${ogg-mingw_SOURCE_DIR}/mingw32/lib
      -L${png-mingw_SOURCE_DIR}/mingw32/lib
      -L${vpx-mingw_SOURCE_DIR}/mingw32/lib
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
