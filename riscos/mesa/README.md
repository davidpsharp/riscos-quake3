# riscos-mesa experiments

Quake III's time on a Pi 4 is almost all riscos-mesa's span and texel code
(see the top README). These are the tools for trying changes there.

Build riscos-mesa from source (the tag matching the devkit), with its
source tree in a Docker volume (Mesa's tarball doesn't unpack onto a macOS
bind mount), then ioquake3 against it:

```sh
git clone -b v20.3.5-16 https://github.com/adyoull/riscos-mesa build/mesa-src
docker volume create q3-mesa-src
(cd build/mesa-src && docker run --rm --platform linux/amd64 -v "$PWD":/src \
   -v q3-mesa-src:/src/src -w /src q3-riscos bash -c \
   'export GCCSDK_ENV=$RISCOS_TOOLCHAIN; build/build-zlib.sh && build/build-mesa.sh')
riscos/sdk.sh riscos/build-mesa-variant.sh   # build/riscos-mesa/RunImage,ff8
```

Unchanged, it matches the devkit (480x300 timedemo: 29.8 against 29.7 fps).

Tried on `ro_group_bgr_modulate` (21% of the time), both bit-exact:

| Change | Script | 480x300 fps |
| --- | --- | --- |
| none | | 29.7 |
| one 8-byte load per pixel and row, channels by `vtbl4`/`vtbx4` | `apply-pairgather.py` | 26.0 (slower) |
| the sixteen `vld3` lane loads as four chains of four, ORed | `apply-splitchain.py` | 29.9 (no change) |

So the lane loads' dependency chain isn't what limits it; the texel
reads themselves (memory) are the likelier cost.

## This port's patches (v0.1.1)

`riscos/mesa/build.sh` builds riscos-mesa from source with these applied
(each script patches one file, once; `src/.q3-patches` in the volume records
which are in), and `riscos/cmake/SDL2` links the result when it exists:

| Script | Change | 480x300 fps (Pi 4) |
| --- | --- | --- |
| `apply-lodbits.py` | `fast_tex_image`: the mip level from the float exponent of 2 rho² instead of two `sqrtf` and a `log2f` (UnixLib C functions) a triangle | 31.1 → 31.3 |
| `apply-vfpround.py` | `_mesa_lroundevenf` as `vcvtr.s32.f32` instead of UnixLib's C `lrintf` | no change on the Pi 4; the Pi 3 gained with the two together (20.7 → 21.3) |
| `apply-fixedsetup.py` | the triangle setup's `FloatToFixed` (about 100,000 a frame, 2% of the time in UnixLib's `lroundf`) through `ro_float_to_fixed` | 32.3 → 32.3 on its own |
| `apply-fixedvfp.py` | `ro_float_to_fixed` as one `vcvtr.s32.f32` (ties round to even rather than away from zero: a pixel at a triangle's edge now and then) | 32.3 → 33.2 |

The build is tuned for the Cortex-A53 (`build/mesa-src-a53`): Pi 3 20.0 →
20.7 fps from the tuning alone, and the Pi 4 is quicker with it too (31.7
against 31.4 with `-mtune=cortex-a72`, which `MESA_TUNE=cortex-a72` builds in
`build/mesa-src`). Tried and
dropped: the depth-first route engaging at a 5% failure share instead of 20%
(`RO_ZF_SHARE`), and a NEON copy into the overlay (EGL).
