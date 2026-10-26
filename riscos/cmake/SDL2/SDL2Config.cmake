# SDL 2.26 from the riscos-mesa devkit ($RISCOS_DEVKIT), linked statically.
# The devkit's SDL2 brings OpenGL through EGL and OSMesa, so those come too.
# OSMESA_LIBRARY (cmake -D) links another libOSMesa.a, e.g. one built from
# riscos-mesa's source with changes.
set(_devkit $ENV{RISCOS_DEVKIT})
if(NOT OSMESA_LIBRARY)
  set(OSMESA_LIBRARY ${_devkit}/lib/libOSMesa.a)
endif()
set(SDL2_VERSION 2.26.0)
set(SDL2_INCLUDE_DIRS ${_devkit}/include/SDL2 ${_devkit}/include)
set(SDL2_LIBRARIES SDL2::SDL2)
if(NOT TARGET SDL2::SDL2)
  add_library(SDL2::SDL2 STATIC IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION ${_devkit}/lib/libSDL2.a
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS _REENTRANT
    INTERFACE_LINK_LIBRARIES "${_devkit}/lib/libGLU.a;${_devkit}/lib/libEGL.a;${OSMESA_LIBRARY};stdc++;${_devkit}/lib/libz.a;m")
endif()
set(SDL2_FOUND TRUE)
