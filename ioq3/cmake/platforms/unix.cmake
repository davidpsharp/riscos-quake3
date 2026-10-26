# Unix specific settings (this includes macOS and emscripten)

if(NOT UNIX)
    return()
endif()

list(APPEND SYSTEM_PLATFORM_SOURCES ${SOURCE_DIR}/sys/sys_unix.c)

if(EMSCRIPTEN OR RISCOS)
    # RISC OS: a desktop program has no terminal on stdin
    list(APPEND SYSTEM_PLATFORM_SOURCES ${SOURCE_DIR}/sys/con_passive.c)
else()
    list(APPEND SYSTEM_PLATFORM_SOURCES ${SOURCE_DIR}/sys/con_tty.c)
endif()

if(USE_HTTP)
    list(APPEND CLIENT_PLATFORM_SOURCES ${SOURCE_DIR}/client/cl_http_curl.c)
endif()

if(RISCOS)
    # Everything is linked statically: UnixLib has no dynamic loader
    list(APPEND COMMON_LIBRARIES m)
else()
list(APPEND COMMON_LIBRARIES
    ${CMAKE_DL_LIBS}    # Dynamic loader
    m                   # Math library
)
endif()
