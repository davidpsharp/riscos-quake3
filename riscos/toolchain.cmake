# RISC OS port, copyright David Sharp 2026
# CMake toolchain for ioquake3 on RISC OS 5 (ARMv7 with NEON: Raspberry Pi 2 and later),
# riscos-crossdev's toolchain file plus the riscos-mesa devkit's SDL2 and OpenAL.
include($ENV{RCD_DIR}/cmake/riscos.cmake)

# NEON (with VFPv3, so Cortex-A8/A9 boards still run it), tuned for the Pi 4.
# RISC OS traps unaligned accesses; the compiler never generates them by default.
set(FPU neon)
set(CMAKE_C_FLAGS_INIT "-mfpu=neon -mfloat-abi=hard -mtune=cortex-a72 -fstack-clash-protection")

# The devkit has no CMake package for SDL2: riscos/cmake/SDL2 describes it.
set(SDL2_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake/SDL2 CACHE PATH "")
set(OPENAL_INCLUDE_DIR $ENV{RISCOS_DEVKIT}/include CACHE PATH "")
set(OPENAL_LIBRARY $ENV{RISCOS_DEVKIT}/lib/libopenal.a CACHE FILEPATH "")
