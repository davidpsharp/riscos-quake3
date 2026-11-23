# Backlog

Not started; waiting for a go-ahead.

- **Move the Mesa speed patches to the -22 devkit's Mesa.** The main line (the GPU build) links
  riscos-mesa's stock software libOSMesa. `riscos/mesa/build.sh` only builds for the -16
  devkit, and the cmake looks for `build/mesa-r3d*`, which nothing makes. So software
  rendering (Pi 3, or a Pi 4 without RISC3D) may be slower than v0.1.2. On the GPU it makes
  no difference.
- **Benchmark the main line on a Pi 3 against v0.1.2.** If it's slower, point software-only
  users at v0.1.2 in the GitHub release notes until the patches are moved.
- **RTCW's console-key fix.** Take it over from riscos-rtcw and test it full screen.
