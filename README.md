# ioquake3 for RISC OS

A port of [ioquake3](https://github.com/ioquake/ioq3) (Quake III Arena's engine)
to RISC OS 5 on ARMv7 (Raspberry Pi 2 and later; tuned for the Pi 4), drawing with
[riscos-mesa](https://github.com/adyoull/riscos-mesa)'s software OpenGL through SDL2.

It plays: the Quake III Arena demo runs from its own menus, full screen, with
mouse and keyboard, bots, and the internet server browser. On a Pi 4 a bot match
runs at about 30 to 35 frames a second, drawing 480x300 and scaled up to the
screen by the Pi's display hardware.

- `ioq3/`: ioquake3 at upstream commit 87a99d1, with the RISC OS changes
  (all under `__riscos__` or `if(RISCOS)`).
- `riscos/`: the cross-build (Docker image `q3-riscos`, from riscos-crossdev),
  the application (`app/!ioquake3`), and the benchmark and profiling tools.

## Building

```sh
docker build --platform linux/amd64 -t q3-riscos riscos/docker
riscos/sdk.sh riscos/build.sh      # build/riscos/RelWithDebInfo/ioquake3 (ELF, with symbols)
riscos/sdk.sh riscos/package.sh    # build/riscos/app/!ioquake3 and build/riscos/ioquake3-riscos.zip
```

The game code runs as QVM bytecode (through ioquake3's ARM JIT). The demo's own
QVMs (1999) are too old for ioquake3, so the package carries ioquake3's, in
`demoq3/zz-ioq3-vm.pk3`. They're platform independent; build them with a host
build of ioquake3 (`-DBUILD_GAME_QVMS=ON`, which builds q3lcc and q3asm) and zip
`baseq3/vm` into `data/qbase/demoq3/zz-ioq3-vm.pk3` before packaging.

## Playing

Put the game data in `!ioquake3`: the free demo's `pak0.pk3` in `demoq3`, or the
full game's `pak0`-`pak8` in `baseq3`. `!Run` uses `baseq3` when it's there.
`!ioquake3.!Help` has the details and the speed settings.

## The RISC OS changes

| Area | Change |
| --- | --- |
| Platform | `q_platform.h` section for `__riscos__`; no `libdl`; static link; passive console (stdin isn't a terminal) |
| VM | The ARM JIT compiles into `malloc`'d memory and calls `OS_SynchroniseCodeAreas` (no `mmap`/`mprotect`) |
| Network | IPv4 only by default (RISC OS has no IPv6); no `ifaddrs` |
| Timer | `Sys_Milliseconds` from `OS_ReadMonotonicTime` plus HAL counter 0: UnixLib's `gettimeofday` counts in centiseconds, which made frame timing jump in 10 ms steps |
| Renderer | `r_glFastest` (`GL_PERSPECTIVE_CORRECTION_HINT`, `GL_FASTEST`) keeps mipmapped textures on riscos-mesa's NEON route; vertex lighting by default |
| Display | `r_renderHeight`/`r_renderWidth`/`r_overlay` set riscos-mesa SDL's render size and overlay; full screen is SDL's multitasking full screen window, which the overlay needs; the renderer takes the GL drawable's size |
| Input | riscos-mesa's SDL has no relative mouse mode or warp: full screen, the game reads the mouse (`OS_Word 21,4`) and puts it back in the middle (`21,3`, `21,5`). A full screen window counts as focused (SDL never gives input focus) |
| UI | `q3_ui` recognises the demo (`com_basegame demoq3`): demo menus, no CD key. Error dialogs are Wimp error boxes |
| Tools | `com_fpsLog N` (averaged fps and per-part times every N seconds); `r_speeds 1` counts draw calls and texture binds; `Q3Prof` sampling profiler (below) |

## Performance (Raspberry Pi 4, RISC OS 5.30, riscos-mesa 20.3.5-16)

`riscos/bench.sh` times `demos/bench.dm_71`: 833 frames of a 4-bot match on
q3dm1, recorded spectating a bot (made headless with a Linux build under Xvfb).

| Configuration | fps |
| --- | --- |
| 640x480 window, stock ioquake3 settings (lightmaps) | 3.3 |
| 320x240 window, stock settings | 10.5 |
| 320x240 window, vertex lighting | 41 to 47 |
| 400x300, vertex lighting, `r_glFastest 0` (no hint) | 11.6 |
| 400x300, vertex lighting, `r_glFastest 1` | 37.0 |
| 480x300 window, vertex lighting | 29.4 |
| Full screen 1680x1050, drawn at 480x300, screen mode change (CPU stretches) | 25.4 |
| Full screen 1680x1050, drawn at 480x300, full screen window + overlay (**default**) | 28.8 |
| Same, drawn at 384x240 (`r_renderHeight 240`) | 38.5 |

Live play (`riscos/play.sh`, 4 bots, `com_fpsLog`): q3dm1 at the defaults about
29 fps (23 to 35); a 10 minute soak on q3dm17 averaged 35.7 fps, with no crash.
The server, game code and bots take about 1.7 ms a frame.

Settings measured one at a time at 480x300: `r_fastsky 1` +12%;
`r_textureMode GL_NEAREST_MIPMAP_NEAREST` +18%; `cg_drawGun 0` +4%;
`r_znear 8` +2%; `r_picmip`, `cg_marks`, `cg_shadows`, `r_flares`,
`r_dynamiclight`, `r_detailtextures`, `r_ext_multitexture` and `-O3` made no
measurable difference; `r_textureRGBA 1` (four-byte texels) was 13% slower.

### Where the time goes

- With drawing switched off (`r_primitives -1`) a frame takes 2.7 ms at 480x300:
  the engine, cgame, clear and present. Mesa's drawing is the other ~31 ms.
- Per frame: about 66 draw calls, 60 texture binds, 4,500 triangles and 28,000
  spans averaging 12 pixels (`r_speeds 1`, `MESA_STATS`). 99% of spans take
  riscos-mesa's fast route; 83% of fragments pass the depth test.
- The sampling profiler (`*Set Q3Prof <file>`, then `riscos/profsym.py`), at the
  defaults: riscos-mesa's bilinear BGR texel groups `ro_group_bgr_modulate` 21%,
  `ro_neon_affine_linear` 9%, `ro_neon_depth` 7%, `affine_textured_triangle` 6%,
  `_swrast_write_rgba_span` 4%, `_swrast_depth_test_span` 4%, `memcpy` 3%,
  `_mesa_get_format_bytes` 1.4%, `lroundf` 1.9%. Quake's own code and Mesa's
  vertex transform and clipping hardly appear.

So what remains is riscos-mesa's per-span and per-texel work. The 3-byte texel
gather (16 `vld3` lane loads per four pixels) and per-span overheads such as
`_mesa_get_format_bytes` are the places to look there.

## Known issues

- Sound: OpenAL initialises, but the test Pis have neither SharedSoundBuffer nor
  DigitalRenderer, so it hasn't been heard.
- Rorth's "in use by a remote testing agent" notice appears when a remote agent
  moves the mouse while the game has it (the game moves the pointer back), and
  covers the overlay while it's up. Rorth's screenshots can't see the overlay
  (they show black); people at the screen see the game.
- Aborts in UnixLib's handlers (`__h_cback`, `__h_event`, `__h_sigvtalrm`) are
  sometimes recorded in other tasks (ShareFS, ROTHelper) around a UnixLib
  program's exit, or when the machine is reset under it. The machine carries on.
  Other projects' UnixLib programs left the same pattern on these Pis.
- The 24 MB WimpSlot holds only the program; the 128 MB hunk comes from
  UnixLib's heap in a dynamic area.
