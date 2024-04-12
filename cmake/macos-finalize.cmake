if(TARGET_ARCH STREQUAL "universal")
  # Find Dependencies
  find_program(AXBREW axbrew -v)
  if(BREW)
    message(NOTICE "X86-64 Homebrew installation detected")
    set(CMAKE_PREFIX_PATH "/usr/local/homebrew")
  else()
    message(FATAL_ERROR "X86-64 Homebrew not installed: https://medium.com/mkdir-awesome/how-to-install-x86-64-homebrew-packages-on-apple-m1-macbook-54ba295230f")
  endif()  

  get_target_property(INCLUDES ${PROJECT_NAME} INCLUDE_DIRECTORIES)
  get_target_property(LIBRARIES ${PROJECT_NAME} LINK_LIBRARIES)

  add_executable(${PROJECT_NAME}.x86
    ${SRC_FILES}
    ${SRC_SDL}
    ${SRC_GFX}
  )

  target_include_directories(${PROJECT_NAME}.x86
    PRIVATE
    ${INCLUDES}
  )

  target_link_libraries(${PROJECT_NAME}.x86
    PRIVATE
    ${LIBRARIES}
  )

  set_target_properties(${PROJECT_NAME}.x86
    PROPERTIES
    OSX_ARCHITECTURES "x86_64"
    LINK_DIRECTORIES ${CMAKE_PREFIX_PATH}/lib
    LINK_FLAGS -headerpad_max_install_names
  )

  # Distribution Preperation
  add_custom_target(${PROJECT_NAME}.universal ALL
    lipo -create -output ${PROJECT_NAME}.universal ${PROJECT_NAME}.x86 ${PROJECT_NAME}
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Frameworks/native
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Frameworks/others
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/MacOS
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Logs
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Paks
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Saves
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/ScreenShots
    COMMAND cp -a ../engine/resources/PkgInfo ../engine/releases/DARWIN/OpenBOR.app/Contents/
    COMMAND cp -a ../engine/resources/Info.plist ../engine/releases/DARWIN/OpenBOR.app/Contents/
    COMMAND cp -a ../engine/resources/OpenBOR.icns ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/
    COMMAND cp -a ${PROJECT_NAME}.universal ../engine/releases/DARWIN/OpenBOR.app/Contents/MacOS/${PROJECT_NAME}
  )

  add_dependencies(${PROJECT_NAME}.universal ${PROJECT_NAME}.x86 ${PROJECT_NAME})

  add_custom_command(TARGET ${PROJECT_NAME}.universal
    POST_BUILD
    COMMAND ./darwin.sh
    WORKING_DIRECTORY ../engine
  )
else()
  add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Frameworks/native
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/MacOS
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Logs
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Paks
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Saves
    COMMAND mkdir -p ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/ScreenShots
    COMMAND cp -a ../engine/resources/PkgInfo ../engine/releases/DARWIN/OpenBOR.app/Contents/
    COMMAND cp -a ../engine/resources/Info.plist ../engine/releases/DARWIN/OpenBOR.app/Contents/
    COMMAND cp -a ../engine/resources/OpenBOR.icns ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/
    COMMAND cp -a ${PROJECT_NAME} ../engine/releases/DARWIN/OpenBOR.app/Contents/MacOS/${PROJECT_NAME}
  )
  add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ./darwin.sh
    WORKING_DIRECTORY ../engine
  )
endif()
