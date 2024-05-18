if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "NintendoWii")
  message(NOTICE "Invoke DevkitPro's cmake utility directly:")
  message(NOTICE "\t$ENV{DEVKITPRO}/portlibs/wii/bin/powerpc-eabi-cmake -DBUILD_WII=ON ..\n")
endif()

set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-maybe-uninitialized -Wno-stringop-truncation -Wno-enum-int-mismatch -Wno-array-bounds -Wno-stringop-overflow -Wno-address")

set(USE_TREMOR ON)
set(USE_WEBM   ON)

add_definitions(
  -DWII
)

file(GLOB SRC_WII "engine/wii/*.c" "engine/wii/*.h")
target_sources(${PROJECT_NAME} PRIVATE ${SRC_WII})

target_include_directories(${PROJECT_NAME} PRIVATE 
  engine/wii
  $ENV{DEVKITPRO}/portlibs/ppc/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  wupc
  wiiuse
  bte
  fat
  asnd
  ogc
)

# DevkitPro
ogc_create_dol(${PROJECT_NAME})

# Distribution Preperation
add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WII/Logs
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WII/Paks
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WII/Saves
  COMMAND ${CMAKE_COMMAND} -E make_directory ../engine/releases/WII/ScreenShots
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.dol ../engine/releases/WII/boot.dol
)
