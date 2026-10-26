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
