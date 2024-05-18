set(COMMON_COMPILER_FLAGS "${COMMON_COMPILER_FLAGS} -Wno-address -Wno-enum-int-mismatch -Wno-stringop-truncation -Wno-maybe-uninitialized -Wno-deprecated-declarations -Wno-overflow -Wno-shift-count-overflow -Wno-pointer-to-int-cast -fstack-protector-all")
set(ARCH_SUFFIX "")

set(USE_SDL     ON)
set(USE_OPENGL  ON)
set(USE_LOADGL  ON)
set(USE_GFX     ON)
set(USE_VORBIS  ON)
set(USE_WEBM    ON)
set(USE_PTHREAD ON)

set(ENABLE_STATIC     ON)
set(ENABLE_STATIC_SDL ON)

add_definitions(-DWIN -DELF)

# Extract the running MSYS2 environment
if(TARGET_ARCH STREQUAL "unknown")
  string(TOLOWER "$ENV{MINGW_CHOST}" TARGET_ARCH)
endif()

if(TARGET_ARCH MATCHES "arm64")
  set(ARCH_SUFFIX "-arm64")
elseif(TARGET_ARCH MATCHES "64")
  set(ARCH_SUFFIX "-x64")
  add_definitions(-DAMD64)
elseif(TARGET_ARCH MATCHES "86")
  set(ARCH_SUFFIX "-x86")
else()
  message(NOTICE "Supported TARGET_ARCH=[X86|AMD64]")
  message(FATAL_ERROR "Unsupported Docker Architecture")
endif()
