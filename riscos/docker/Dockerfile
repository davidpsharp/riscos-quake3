# RISC OS port, copyright David Sharp 2026, GPL 2
# Cross-build environment for ioquake3 on RISC OS 5 (ARMv7, Raspberry Pi):
# riscos-crossdev 1.11 (GCCSDK GCC 10.2, UnixLib 5.0.3.7) with riscos-mesa's
# 20.3.5-22 devkit in place of the one it carries (20.3.5-18): Mesa OpenGL 2.1
# in software, the GPU through RISC3D (libMesaGPU for a native Pi 4), SDL 2.26,
# OpenAL Soft, zlib.
#   docker build --platform linux/amd64 -t q3-riscos-r3d riscos/docker
FROM ghcr.io/adyoull/riscos-crossdev:1.11
ARG MESA_DEVKIT=20.3.5-22
ARG MESA_DEVKIT_SHA256=8559f004f13192b2d0438e0c9fe707057f49c0b24fe042ba7575c8ae31daf68f
RUN curl -fsSL -o /tmp/devkit.tgz \
      https://github.com/adyoull/riscos-mesa/releases/download/v$MESA_DEVKIT/riscos-mesa-devkit-$MESA_DEVKIT.tgz \
 && echo "$MESA_DEVKIT_SHA256  /tmp/devkit.tgz" | sha256sum -c - \
 && rm -rf /opt/riscos-crossdev/devkit/env \
 && mkdir /opt/riscos-crossdev/devkit/env \
 && tar xzf /tmp/devkit.tgz -C /opt/riscos-crossdev/devkit/env --strip-components=1 \
 && rm /tmp/devkit.tgz \
 && grep -q "$MESA_DEVKIT" /opt/riscos-crossdev/devkit/env/VERSION \
 && test -f /opt/riscos-crossdev/devkit/env/lib/libMesaGPU.a
WORKDIR /work
