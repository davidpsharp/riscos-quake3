# ioquake3 for RISC OS

A port of [ioquake3](https://github.com/ioquake/ioq3) (Quake III Arena's engine)
to RISC OS 5 on ARMv7 (Raspberry Pi 2 and later; tuned for the Pi 4), drawing with
[riscos-mesa](https://github.com/adyoull/riscos-mesa)'s software OpenGL through SDL2.

- `ioq3/`: ioquake3 at upstream commit 87a99d1, with the RISC OS changes.
- `riscos/`: the cross-build (Docker image `q3-riscos`, from riscos-crossdev), the
  application (`app/!ioquake3`), and the benchmark driver.

Building:

```sh
docker build --platform linux/amd64 -t q3-riscos riscos/docker
riscos/sdk.sh riscos/build.sh      # build/riscos/RelWithDebInfo/ioquake3 (ELF)
riscos/sdk.sh riscos/package.sh    # build/riscos/app/!ioquake3
```
