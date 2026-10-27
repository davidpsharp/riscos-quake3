# SDL 2.26 for the RISC OS port

ioquake3 links SDL 2.26.0 with riscos-mesa's RISC OS drivers, built here from
source so the port can carry fixes riscos-mesa doesn't have yet (the same
arrangement as the DevilutionX port, whose build and first two patches these
are):

```sh
riscos/sdk.sh riscos/sdl2/build.sh   # build/riscos-sdl2/stage
```

`riscos/cmake/SDL2` then uses `build/riscos-sdl2/stage` instead of the devkit's
`libSDL2.a` (configure the game's build directory again after building SDL the
first time).

- `overlay/` is riscos-mesa's RISC OS overlay, unchanged: `overlay/SOURCE` says
  which commit. Update it by copying it again from riscos-mesa
  (`tools/sdl-overlay-export.sh`), never by editing it.
- `patches/` are changes on top, to be offered to riscos-mesa:
  - `01-riscos5-sprite-mode-word.patch` (DevilutionX's): a desktop window on a
    64K or 16M colour desktop was plotted at half width and double height.
  - `02-iconbar-menu-info-help.patch` (DevilutionX's): the icon bar menu has
    Info (an "About this program" window), Help (opens the application's
    `!Help`) and Quit. The Info fields come from the hints
    `SDL_RISCOS_APP_PURPOSE`, `SDL_RISCOS_APP_AUTHOR`,
    `SDL_RISCOS_APP_PORTED_BY` and `SDL_RISCOS_APP_VERSION`, which
    `ioq3/code/sdl/sdl_glimp.c` sets.
  - `03-app-dir-as-program-name.patch` (this port's): when the program's name
    is the application directory itself, which is what UnixLib gives a
    program started by its `!Run`, it is taken as the application. SDL took
    its parent instead, so the task was called `!RunImage`, the icon bar icon
    was the generic one and Help found no `!Help`.

To change a patch: build once, edit the file in
`build/riscos-sdl2/SDL-release-2.26.0`, and make the patch again with `diff -u`
against the file as the overlay and the earlier patches leave it (paths
`a/src/...` and `b/src/...`).
