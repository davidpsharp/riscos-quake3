# ioquake3 on RISC3D: results and issues

Summary for RISC3D's author, all three ports: [RISC3D-for-Andy.md](https://github.com/davidpsharp/riscos-dhewm3/blob/main/docs/RISC3D-for-Andy.md). This file is the Quake 3 evidence.

## Results

All runs are `riscos/bench.sh`: 833 frames of a four-bot match on q3dm1. The build is static,
made with crossdev 1.11 and riscos-mesa devkit 20.3.5-22 (`-Wl,-u,riscos_mesa_gpu -lMesaGPU`).

| Machine | Drawing | Size | fps |
|---|---|---|---|
| Pi 4 (pi4-plato, RISC OS 5.30) | software Mesa, software defaults | 480x300 window | 35 |
| **Pi 4 (pi4-socrates, RISC OS 5.30), stock RISC3D 0.17 (V3DRender 0.26) after DAClamp** | GPU, ioquake3's own defaults | 480x300 window | **138** |
| Pi 4, same | GPU | 1024x768 window | 77 |
| Pi 4, same | GPU | 1280x800 full screen | 69 |
| RoMac 0.1.230 (Apple M3 Pro, Linux3D) | GPU, software defaults | 480x300 window | 48.5 |
| RoMac, same | GPU, ioquake3's own defaults | 480x300 window | 70.7 |
| RoMac, same | GPU, ioquake3's own defaults | 960x720 window | 61.8 |

- **RISC3D 0.17 runs Quake 3 with no patches** (tested 23 Nov 2026 on pi4-socrates). It was
  loaded by hand: `DAClamp`, then `Obey !RISC3D.!Boot`.
  - Every run had 0 BO failures, 0 resets and 0 MMU errors.
  - The 4,096-handle limit that blocks Doom 3 doesn't touch Quake 3: a whole session made
    about 1,900 buffers.
  - **On a 64K colour desktop, a window is garbled; full screen is fine.** Same as
    [riscos-mupen64plus item 1](https://github.com/davidpsharp/riscos-mupen64plus/blob/main/docs/RISC3D-0.17-report.md#1-64k-colour-desktops-the-gl-window-shows-the-wrong-pixels-new).
- **On RoMac**, a frame at 480x300 costs nearly as much as one at 960x720. The time goes on the
  cost of each GL call, not on pixels: Quake 3 makes about 90 draws and 80 texture binds a
  frame.
- **Choosing the defaults:** the game decides on its defaults before it makes a GL context,
  because the render size has to be known first. It asks `RISC3D_Info` (SWI &5AF80):
  - R3 is the driver's name, or 0 for none.
  - R4 bit 1 means the driver draws in software.

  It also checks `RISC3D$Disable` and `LIBGL_ALWAYS_SOFTWARE` itself, because of riscos-dhewm3's
  item 1. The code is in `ioq3/code/qcommon/riscos_gpu.h`.

  **Question for Andy:** is that SWI and its flags a supported way to ask, before any context,
  whether the GPU will draw?

## 1. Deleting the window before its GL context aborts the program (riscos-mesa on a GPU)

- **Seen:** every `vid_restart`, on the Pi 4 GPU. stderr shows `MESA: error: wait failed: -9`,
  then UnixLib's "pthread_yield called with context switching disabled" box, and the program
  ends.
- **Cause:** upstream ioquake3's `GLimp_Shutdown` calls `SDL_QuitSubSystem(SDL_INIT_VIDEO)`,
  which destroys the window while the GL context still exists. The context is deleted later, at
  the next `GLimp_SetMode`. On the GPU, Mesa draws into the window's own memory (imported into
  V3DRender), so deleting the context then finishes a frame in freed memory. V3DRender answers
  EBADF (-9) and Mesa aborts.
- **Workaround, in the game:** delete the context, then destroy the window
  (`ioq3/code/sdl/sdl_glimp.c`, `GLimp_Shutdown`). See item 2 for why SDL video is no longer
  quit there.
- **Suggestion:** this order is legal in SDL and harmless on Linux, so other SDL ports will hit
  it. riscos-mesa's SDL could make the context stop drawing to a window when that window is
  destroyed, or Mesa could treat a vanished surface as "nothing to finish" rather than abort.

## 2. Restarting SDL video loses the GPU: the next context is software

- **Seen:** RISC3D 0.17 on pi4-socrates. After a `vid_restart`, the game came back with
  GL_RENDERER "Mesa OffScreen" (Mesa 20.3.5, software) instead of "V3D 4.2.14.0". It ran at
  software speed, and `*V3DRender` showed no file open. With `V3DM$Trace` set, making the new
  context sent the GPU module no requests at all, so riscos-mesa never tried the GPU.
  - This happened after the item 1 fix, which stopped the crash.
  - Earlier `vid_restart` checks on plato and RoMac only looked for a crash, so they would have
    missed it.
- **Cause:** not pinned down. ioquake3's `GLimp_Shutdown` calls
  `SDL_QuitSubSystem(SDL_INIT_VIDEO)`, and `GLimp_SetMode` calls `SDL_Init(SDL_INIT_VIDEO)`
  again. The GPU library's own state (`decide()` in `osmesa/r3d_switch.c`) is kept for the
  life of the program, so the loss is somewhere in SDL's video quit and init.
- **Workaround, in the game:** keep SDL video up across a restart; `SDL_Quit` ends it at exit
  (`ioq3/code/sdl/sdl_glimp.c`). Then `vid_restart`, full screen to a window and a window to full
  screen all stay on the GPU, with 0 resets and 0 MMU errors.
- **Suggestion:** a program that stops and starts SDL video (`SDL_QuitSubSystem`, or `SDL_Quit`
  then `SDL_Init`) should get the GPU again. ioquake3 does it at every `vid_restart`, and other ioquake3-based
  ports (iortcw) will too.

## 3. Relative mouse mode in riscos-mesa's SDL (mouse look)

These were found with RISC3D builds, though they're in SDL, not the GPU stack. They are fixed
by patches in `riscos/sdl2/patches`, which riscos-dhewm3 shares (and riscos-half-life will).
Each patch is against SDL 2.32.10 as riscos-mesa 2df5c85 carries it.

- **06: the motion was counted in OS units, not pixels.** Its speed depended on the screen
  mode's OS units per pixel. SDL now reports screen pixels and carries fractions over.
- **07: the first reading after relative mode starts is a jump.** The pointer is wherever the
  desktop left it, so the view jumped. Now the first reading after relative mode starts, or
  after a pause in reading, only re-centres.
- **08: the view drifted (pitch crept up or down) with the mouse still.** The centre was
  computed in OS units. `OS_Word 21` puts the pointer on whole pixels, so an odd number of OS
  units left the pointer half a pixel away, and every poll read that as a move. Now the
  centre is rounded to a pixel.
- **Suggestion:** take 06 to 08 into riscos-mesa's SDL overlay.

## 4. Rorth screenshots of a GPU window show bands from different frames

- **Seen:** on the Pi 4 GPU, a Rorth screenshot during play shows horizontal bands, each from a
  different frame.
- **Not confirmed on the screen itself.** Nobody has looked at the monitor during play yet. It
  may be only the screenshot reading memory while the GPU writes it, or real tearing.
- **Question for Andy:** does V3DRender, or riscos-mesa's swap, wait for the frame to finish
  before the window's memory is shown, and for vsync?

## 5. Minor

- **Linking libMesaGPU statically** adds about 15 MB: the game's `!RunImage` is 30 MB. That's
  fine, but it means a game won't pick up a Mesa fix without being rebuilt.
