# ioquake3 for RISC OS

Version 0.2.0 (26 Nov 2026). New since 0.1.2: it draws on the GPU through RISC3D
(0.22 or later; a Pi 4's V3D, or RoMac on a Mac) when there's a driver, and in software
when there isn't; and the full game from Steam has been tested.

A port of [ioquake3](https://github.com/ioquake/ioq3) (Quake III Arena's engine)
to RISC OS 5 on ARMv7 (Raspberry Pi 2 and later; tuned for the Pi 4), drawing with
[riscos-mesa](https://github.com/adyoull/riscos-mesa)'s software OpenGL through SDL2.

It plays: the Quake III Arena demo runs from its own menus, full screen or in a
window (Alt+Enter switches), with
mouse and keyboard, bots, and the internet server browser. On a Pi 4 a bot match
runs at about 30 to 35 frames a second, drawing 480x300 and scaled up to the
screen by the Pi's display hardware.

Home page: https://davidsharp.com/riscos/quake3/

Contact (questions, bug reports): https://davidsharp.com/contact/, or raise an
[issue](https://github.com/davidpsharp/riscos-quake3/issues) here.

<p>
<img src="https://github.com/davidpsharp/riscos-quake3/releases/download/risc3d-preview2/screenshot-pi4-rocket-fight.png" width="49%" alt="A bot firing rockets down a corridor in q3dm7, in a window on the RISC OS desktop">
<img src="https://github.com/davidpsharp/riscos-quake3/releases/download/risc3d-preview2/screenshot-pi4-rocket-explosion.png" width="49%" alt="A rocket explosion on the stairs in q3dm7">
<img src="https://github.com/davidpsharp/riscos-quake3/releases/download/risc3d-preview2/screenshot-pi4-lava-pit.png" width="49%" alt="The lava pit in q3dm7">
<img src="https://github.com/davidpsharp/riscos-quake3/releases/download/risc3d-preview2/screenshot-romac-window.png" width="49%" alt="ioquake3 in a window on the RISC OS desktop: the Quake III Arena demo's q3dm1">
<img src="https://github.com/davidpsharp/riscos-quake3/releases/download/v0.2.0/screenshot-romac-full-game-visor.png" width="49%" alt="The full game from Steam in a window on the RISC OS desktop (RoMac): machine-gunning the bot Visor">
<img src="https://github.com/davidpsharp/riscos-quake3/releases/download/v0.2.0/screenshot-romac-full-game-stairs.png" width="49%" alt="The full game on RoMac: a bot coming down the stairs under rocket fire">
</p>

*In a window on the RISC OS desktop. The first three are a bot match on q3dm7 on a Raspberry Pi 4
(RISC3D drawing on its GPU, following a bot with the HUD hidden); the last is q3dm1 on RoMac
(RISC3D drawing on the Mac's GPU).*

- `ioq3/`: ioquake3 at upstream commit 87a99d1, with the RISC OS changes
  (all under `__riscos__` or `if(RISCOS)`).
- `riscos/`: the cross-build (Docker image `q3-riscos-r3d`: riscos-crossdev 1.11 with
  riscos-mesa's 20.3.5-23 devkit),
  the application (`app/!ioquake3`), and the benchmark and profiling tools.

RISC OS port, copyright David Sharp 2026: the files under `riscos/` (except
`riscos/sdl2/overlay`, riscos-mesa's) and the new files `ioq3/code/qcommon/riscos_gpu.h` and
`ioq3/code/sys/sys_riscos_prof.c`. ioquake3's own files keep their notices, with the RISC OS
changes under `__riscos__` or `if(RISCOS)`. The port's files are under the GNU General Public
License version 2 (`ioq3/COPYING.txt`).

## Building

```sh
docker build --platform linux/amd64 -t q3-riscos-r3d riscos/docker
riscos/sdk.sh riscos/sdl2/build.sh # SDL with this port's patches (build/riscos-sdl2-r3d/stage)
riscos/sdk.sh riscos/build.sh      # build/riscos-r3d/RelWithDebInfo/ioquake3 (ELF, with symbols)
riscos/sdk.sh riscos/package.sh    # build/riscos-r3d/app/!ioquake3 and build/riscos-r3d/ioquake3-riscos-VERSION.zip
```

The game links the devkit's libOSMesa (software) and libMesaGPU (the Pi 4's GPU).
`riscos/mesa/build.sh` builds a faster software libOSMesa with this port's patches,
but only for the older 20.3.5-16 devkit (v0.1.2); it isn't brought forward to -23 yet.

The game code runs as QVM bytecode (through ioquake3's ARM JIT). The demo's own
QVMs (1999) are too old for ioquake3, so the package carries ioquake3's, in
`demoq3/zz-ioq3-vm.pk3`. They're platform independent; build them with a host
build of ioquake3 (`-DBUILD_GAME_QVMS=ON`, which builds q3lcc and q3asm) and zip
`baseq3/vm` into `data/qbase/demoq3/zz-ioq3-vm.pk3` before packaging.

## On a GPU (RISC3D)

The game links riscos-mesa's libMesaGPU, so the game draws on a GPU when
RISC3D (Andrew Youll's GPU layer) has a driver: on a Pi 4
(V3DRender) or in RoMac. It then uses ioquake3's own quality defaults (lightmaps,
full resolution). Without a driver it draws in software as before. Results and
the issues seen are in `docs/risc3d-issues.md`.
Tested with RISC3D 0.22 (with !MesaLibs 20.3.5-23 also installed) on a Pi 4, 25 Nov 2026.
The game is linked statically, so it needs only `!RISC3D` in `!Boot.Resources`; from 0.22
that no longer depends on !MesaLibs booting first (RISC3D's release notes).

## Getting the game data

ioquake3 is only the engine. The data:

- **The Quake III Arena demo** (free, four maps): `pak0.pk3` from id's Linux
  demo installer, `linuxq3ademo-1.11-6.x86.gz.sh` (a shell script with a tar
  archive inside: `demoq3/pak0.pk3`), on id's FTP mirrors, e.g.
  <https://ftp.gwdg.de/pub/misc/ftp.idsoftware.com/idstuff/quake3/linux/linuxq3ademo-1.11-6.x86.gz.sh>.
- **The full game**: Quake III Arena from Steam or GOG
  (<https://ioquake3.org/buy/> keeps the links). Copy `pak0.pk3` to `pak8.pk3`,
  and `q3key`, from the installed game's `baseq3` folder (Steam:
  `steamapps/common/Quake 3 Arena/baseq3`) into a `baseq3` folder in
  `!ioquake3`. Steam's copy includes id's point release data (`pak1` to `pak8`,
  1.32); a CD copy has only `pak0`, and the rest is free from
  <https://ioquake3.org/extras/patch-data/> (`quake3-latest-pk3s.zip`). Without
  `q3key` the game asks for a CD key at each start, and Escape skips it. Leave
  out the PC's `q3config.cfg`. Tested with Steam's files in RoMac, 26 Nov 2026:
  the intro movie, full menus and a map the demo hasn't (q3dm6) with bots.
  Team Arena (`missionpack`) hasn't been tried.
- **OpenArena** (free, GPL and CC licensed): 0.8.8's `baseoa` pk3 files, from
  <https://openarena.ws> or its mirror,
  <https://download.tuxfamily.org/openarena/rel/088/openarena-0.8.8.zip>.

id's data can't be put in this package: its licence allows only
non-commercial copies of the demo, with the licence, labelled as a demo.

## Playing

Put the game data in `!ioquake3`: the free demo's `pak0.pk3` in `demoq3`, or the
full game's `pak0`-`pak8` in `baseq3`, or OpenArena 0.8.8's `baseoa` pk3s in
`baseoa`. `!Run` uses `baseq3`, else `baseoa`, else `demoq3`.

With none of them, `!Run` starts the launcher (`riscos/launcher`, adapted from
riscos-doom3's). It offers to fetch the demo: id's Linux installer from a mirror
of id's FTP site over HTTP, checked by SHA-256; id's licence is unpacked and
shown, and `demoq3/pak0.pk3` (also checked) is installed only if the user
accepts it. Then it starts the game through `!Run` and quits. Dropping the
installer on its icon does the same without the download.
`!ioquake3.!Help` has the details and the speed settings.

## The RISC OS changes

| Area | Change |
| --- | --- |
| Platform | `q_platform.h` section for `__riscos__`; no `libdl`; static link; passive console (stdin isn't a terminal) |
| VM | The ARM JIT compiles into `malloc`'d memory and calls `OS_SynchroniseCodeAreas` (no `mmap`/`mprotect`) |
| Network | IPv4 only by default (RISC OS has no IPv6); no `ifaddrs` |
| Timer | `Sys_Milliseconds` from `OS_ReadMonotonicTime` plus HAL counter 0: UnixLib's `gettimeofday` counts in centiseconds, which made frame timing jump in 10 ms steps |
| Renderer | `r_glFastest` (`GL_PERSPECTIVE_CORRECTION_HINT`, `GL_FASTEST`) keeps mipmapped textures on riscos-mesa's NEON route; vertex lighting and no multitexture by default, put back after a game's `default.cfg` (OpenArena's turns vertex lighting off) |
| Display | `r_renderHeight`/`r_renderWidth`/`r_overlay` set riscos-mesa SDL's render size and overlay; full screen is SDL's multitasking full screen window, which the overlay needs; the renderer takes the GL drawable's size |
| Input | Mouse look through SDL's relative mouse mode, with this port's fixes to riscos-mesa's SDL (`riscos/sdl2/patches` 04-08: RoMac's pointer, motion in screen pixels, no jump when it starts, the centre on a whole pixel). A full screen window counts as focused (SDL never gives input focus) |
| UI | `q3_ui` recognises the demo (`com_basegame demoq3`): demo menus, no CD key. Error dialogs are Wimp error boxes |
| Desktop | Icon (`!Sprites`, `!Sprites22`; drawn by `riscos/tools/make_icon.py`); the icon bar menu has Info (version, authors), Help and Quit, from the SDL in `riscos/sdl2` (riscos-mesa's, with DevilutionX's menu patch and a fix for the task's name) |
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
| 480x300 window, lightmaps, `r_ext_multitexture 1` (two texture units: general route) | 6.1 |
| 480x300 window, lightmaps, `r_ext_multitexture 0` (two passes, both fast route) | 16.5 |
| 384x240 window, lightmaps, `r_ext_multitexture 0` | 21.8 |
| Full screen 1680x1050, drawn at 480x300, screen mode change (CPU stretches) | 25.4 |
| Full screen 1680x1050, drawn at 480x300, full screen window + overlay (**default**) | 28.8 |
| Same, drawn at 384x240 (`r_renderHeight 240`) | 38.5 |

On a **Raspberry Pi 3** (Cortex-A53), which defaults to 200 lines (the game
reads the CPU's ID register: a Cortex-A72 or later gets 300): 480x300 window
12.5 fps, 384x240 16.2, 320x200 19.7 (23.7 with `r_fastsky 1` and nearest
filtering), full screen 1680x1050 at 320x200 19.7. With v0.1.1 (below) full
screen at 320x200 is 23.5; the Pi 4's full screen default is 36.1 (was 28.8).

Live play (`riscos/play.sh`, 4 bots, `com_fpsLog`): q3dm1 at the defaults about
29 fps (23 to 35); a 10 minute soak on q3dm17 averaged 35.7 fps, with no crash.
The server, game code and bots take about 1.7 ms a frame.

OpenArena 0.8.8 (`baseoa`, its own QVMs, standalone) plays too, slower: its
maps draw about 4 layers a pixel against Quake III's 2.4. Live on oa_dm1 with 4
bots at 480x300: 5 to 7 fps as OpenArena configures itself (lightmaps), 6 with
vertex lighting (55% of fragments went down riscos-mesa's general route: its
two-stage shaders were collapsed onto two texture units), and 12 to 21 fps with
`r_ext_multitexture 0` as well, everything on the fast route. Both are now
the RISC OS defaults. A 10 minute soak on oa_dm2, a heavier map, full screen at
the defaults, averaged 10.9 fps (4 to 16), with no crash. `r_ext_multitexture 0` makes no difference to Quake III with
vertex lighting, and makes its lightmaps 2.7 times as fast (table above).

Settings measured one at a time at 480x300: `r_fastsky 1` +12%;
`r_textureMode GL_NEAREST_MIPMAP_NEAREST` +18%; `cg_drawGun 0` +4%;
`r_znear 8` +2%; `r_picmip`, `cg_marks`, `cg_shadows`, `r_flares`,
`r_dynamiclight`, `r_detailtextures`, `r_ext_multitexture` (on Quake III with
vertex lighting) and `-O3` made no
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

### Second round: draw order, rounding, tuning (v0.1.1)

riscos-mesa tests depth before it textures (its `zfirst` route), so the order
things are drawn in decides how much texturing is wasted on fragments that
end up hidden. Measured on the 480x300 window benchmark, Pi 4 (plato) and the
320x200 one on the Pi 3, each change on top of the last:

| Change | Pi 4 fps | Pi 3 fps |
| --- | --- | --- |
| v0.1.0 | 28.9 | 19.7 |
| `r_skyLast 1`: the sky after the opaque world (it was drawn first; now the world hides most of it). Fragments passing the depth test 83% → 65% | 30.5 | 20.2 |
| `r_sortDepth 32`: opaque surfaces nearest first, each shader placed by its nearest surface (same texture binds). Passing fragments → 57% with per-surface order, but that costs 60 more texture binds a frame (`r_sortDepthSurfaces 1`: 30.9), about 20 µs each in Mesa | 31.1 | 20.4 |
| `Q_ftol` as one VFP instruction (`vcvtr`) instead of UnixLib's C `lrintf` | (in the above) | (in the above) |
| riscos-mesa built from source with `fast_tex_image`'s mip level from the float exponent (no `sqrtf`/`log2f` a triangle) and `_mesa_lroundevenf` as `vcvtr` (`riscos/mesa/apply-lodbits.py`, `apply-vfpround.py`) | 31.3 | — |
| The same Mesa built `-mtune=cortex-a53` (`riscos/mesa/build.sh`'s default: faster on the Pi 4 too, 31.7 twice against 31.4, so one build serves both) | 31.7 | 21.3 (20.7 from the tuning alone) |
| `r_subdivisions 8` (curves in fewer triangles, so fewer spans; 12 and 20 gain no more, so the least visible is the default) | 32.3 | — |
| Mesa's `FloatToFixed` in the triangle setup (100,000 UnixLib `lroundf` calls a frame, 2%) as one `vcvtr` (`apply-fixedsetup.py`, `apply-fixedvfp.py`; ties round to even) | 33.2 | 22.3 |
| `r_textureMode GL_NEAREST_MIPMAP_NEAREST_LINEAR` (new mode: one texel instead of four for minified textures, bilinear kept for magnified; no visible change at 480x300. All nearest is 37.6 but blocky up close, and on the Pi 3 no faster: 23.7) | 36.3 | 23.6 |

No measurable change: 16-bit depth buffer (`r_depthbits 16`), the depth-first
route engaging at a 5% failure share instead of 20% (`RO_ZF_SHARE`), a NEON
64-byte-store copy of the frame into the overlay in riscos-mesa's EGL (the
overlay's uncached memory takes `memcpy` just as fast: full screen 30.8 and
31.4 fps either way; not kept). `r_depthbits 32` is
refused by OSMesa (GLimp_Init fails). Slower: `r_primitives 1` and `3`
(strips through `glArrayElement` or immediate mode: 17.7 and 18.3 on the Pi
3 against 20.4), and in Mesa's spans, copying the pixels with NEON loads and
stores instead of UnixLib's `memcpy` (30.9 against 33.2) or filling the mask
with a word loop instead of its `memset` (31.4 against 33.3): the library
calls are the quick way. A NEON version of the nearest-texel span
(`SPAN_NEAREST`, 4.8% of a frame once it was the minifying filter) ran at
the same speed: its cost is the scattered texel loads, not the arithmetic. The Pi 3's
profile (`riscos/prof.sh`, four demo loops) has the same shape as the Pi
4's: the nearest span 5.3%, Mesa's span writer 3.0%, the triangle setup
2.2%, the depth test about 4%, the rest spread thinly through Mesa.

Game-side settings measured at 480x300 on the Pi 4 against a 36.0 control:
`r_dynamiclight 0` 36.3, `cg_shadows 0` 36.6, `cg_marks 0` 36.9,
`cg_simpleItems 1` 36.4, `cg_brassTime 0` 35.9, and the first three together
36.3. Each is at most a frame a second for something visible (muzzle light,
blob shadows, bullet marks), so none is a default. `r_picmip 1` and `2`
(textures at half and quarter size) are 36.0 and 35.7: the texel loads that
the profile shows aren't cache misses. The Pi 3 at 320x200 agrees: 23.6
control, 23.6 and 23.7 with picmip, 24.1 with the three settings together.

OpenArena, live on oa_dm2 full screen for two minutes with v0.1.1: 11.9 fps
average (4 to 17), against the 10.9 (4 to 16) of the ten minute v0.1.0 soak;
its maps' overdraw is what the depth ordering is for, but live play is a
rough measure.

The engine's draw calls can't usefully go below one per visible shader
(about 90 a frame), and each costs Mesa about 20 µs of state validation and
setup, so what the finer depth order saves in texturing it loses in binds.

Also looked at, afternoon of the same day:

- Sound off (`s_initsound 0`, so no OpenAL mixer thread): 36.2 against 35.9
  and 36.1 with it. The mixer and the main thread's yields to it cost nothing
  measurable. (70% of the Q3Prof samples sit at UnixLib's yield return point:
  the sampler takes one each time the main thread yields voluntarily, about 13
  a frame, so that line counts events, not time. The active samples are the
  profile.)
- Profile-guided optimisation of Mesa: not possible with this toolchain. Its
  `libgcov.a` is stubs (every routine a single return, no `__gcov_exit`), so
  an instrumented build doesn't link and would write nothing if it did.
- Mesa compiled `-mfpu=neon-vfpv4` throughout (`MESA_FPU=neon-vfpv4
  riscos/mesa/build.sh`), so GCC may vectorise everywhere rather than only in
  riscos-mesa's hand-written NEON files: 34.7 and 34.9 against 35.8 and 36.1
  for the default `vfpv3` build, alternated on the Pi 4. Slower; not kept.
- riscos-mesa's span writer already has a fused fast route
  (`ro_write_rgba_span_fast`, 99% of spans) and a depth-first one, so the
  "one loop for depth, texel, modulate and write" idea is largely done
  upstream; what remains per pixel is the texel gather and the depth test.

## Known issues

- LAN play works, but has failed twice, both times with plato (one of the
  two Pi 4s) as the client and soon after it had been restarted or power
  cycled. Once it entered the game, then stopped receiving (the server logged
  thousands of "Delta request from out of date packet") and plato stopped
  answering ping and Rorth until it was power cycled. Once, after that power
  cycle, its client failed while loading the map with "Couldn't open
  ./demoq3/pak0.pk3" (the file was there and fine): the game reopens pak0 for
  each streamed file, and that open failed. Cause not yet known. Since then,
  all clean: plato as the client for 16 minutes (about 32 fps, Rorth polling
  it every 8 s for part of that made no difference), 5 quit-and-rejoin
  cycles, and the Pi 3 (about 16 fps) and plato in the same game together.
  Socrates as the client against plato hosting: 3 minutes at about 33 fps.
  The LAN browser's broadcast and connecting worked every time.
- The game finds its own address from the hostname. Both test Pis are called
  RISCOSpi, which resolves to another machine (192.168.1.85 for one at
  .140). It only matters for deciding what is LAN, and private addresses
  count as LAN anyway.

- Sound plays, but no one has listened to it yet. By default OpenAL Soft
  mixes and sends it to UnixLib's `/dev/dsp`, which plays through
  DigitalRenderer (in !System on the test Pis). In a bot match DigitalRenderer
  played 16-bit stereo at 44.1 kHz, about 90 buffers a second, with the game
  at 31 fps on a Pi 4 and 23 to 26 on a Pi 3 (full screen). With
  `s_useOpenAL 0` and SharedSoundBuffer and StreamManager loaded, SDL's
  `riscos` driver plays it instead (22 kHz), also at 31 fps. Whether OpenAL
  gives way to SDL when only SharedSoundBuffer is installed hasn't been
  tried.
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
