if(TARGET_ARCH STREQUAL "universal")
  # Find Dependencies
  find_program(AXBREW /usr/local/homebrew/bin/brew -v)
  
  if(AXBREW)
    message(NOTICE "X86-64 Homebrew installation detected")
    set(CMAKE_PREFIX_UNIVERSAL_PATH "/usr/local/homebrew")
    set(CMAKE_LIBRARY_UNIVERSAL_PATH "${CMAKE_PREFIX_UNIVERSAL_PATH}/lib")
  elseif(NOT CMAKE_PREFIX_UNIVERSAL_PATH)
    message(WARNING "X86-64 Homebrew not installed: https://github.com/SumolX/MacOS-Universal-Binary")
    message(FATAL_ERROR "CMAKE_PREFIX_UNIVERSAL_PATH and CMAKE_LIBRARY_UNIVERSAL_PATH are required.")    
  endif()  

  get_target_property(INCLUDES ${PROJECT_NAME} INCLUDE_DIRECTORIES)
  get_target_property(LIBRARIES ${PROJECT_NAME} LINK_LIBRARIES)

  add_executable(${PROJECT_NAME}.x86
    ${SRC_FILES}
    ${SRC_SDL}
    ${SRC_GFX}
    ${SRC_WEBM}
    ${SRC_OPENGL}
    ${SRC_LOADGL}
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
    LINK_DIRECTORIES ${CMAKE_LIBRARY_UNIVERSAL_PATH}
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
    COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/DARWIN/OpenBOR.app/Contents/Frameworks/native
    COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/DARWIN/OpenBOR.app/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Logs
    COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Paks
    COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/Saves
    COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/ScreenShots
    COMMAND ${CMAKE_COMMAND} -E copy ../engine/resources/PkgInfo ../engine/releases/DARWIN/OpenBOR.app/Contents/
    COMMAND ${CMAKE_COMMAND} -E copy ../engine/resources/Info.plist ../engine/releases/DARWIN/OpenBOR.app/Contents/
    COMMAND ${CMAKE_COMMAND} -E copy ../engine/resources/OpenBOR.icns ../engine/releases/DARWIN/OpenBOR.app/Contents/Resources/
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ../engine/releases/DARWIN/OpenBOR.app/Contents/MacOS/${PROJECT_NAME}
  )
  add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ./darwin.sh
    WORKING_DIRECTORY ../engine
  )
endif()
