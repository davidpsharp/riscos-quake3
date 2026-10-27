#!/usr/bin/env python3
"""Makes a RISC OS sprite file from a PNG.

    make_sprites.py <in.png> <out,ff9> name:WxH[:dpi] [name:WxH[:dpi] ...]

Each sprite is the picture scaled to W x H pixels, in 32 bpp (sprite type 6)
with a 1 bpp mask: pixels with alpha under 128 are transparent. The dpi
(90 by default, 180 for the large icons of high resolution modes) is the same
across and down. RISC OS 5 plots these in any screen mode.
"""
import struct
import sys

from PIL import Image


def sprite(picture, name, width, height, dpi):
    im = picture.convert('RGBA').resize((width, height), Image.LANCZOS)
    px = im.load()
    image = bytearray()
    for y in range(height):
        for x in range(width):
            r, g, b, _ = px[x, y]
            image += bytes((r, g, b, 0))  # each word is &00BBGGRR
    mask_words = (width + 31) // 32
    mask = bytearray()
    for y in range(height):
        row = [0] * mask_words
        for x in range(width):
            if px[x, y][3] >= 128:
                row[x // 32] |= 1 << (x % 32)  # the leftmost pixel is bit 0
        mask += struct.pack('<%dI' % mask_words, *row)
    mode = (6 << 27) | (dpi << 14) | (dpi << 1) | 1
    header_size = 44
    total = header_size + len(image) + len(mask)
    header = struct.pack('<I12s7I', total, name.lower().encode('latin-1')[:12].ljust(12, b'\0'),
                         width - 1, height - 1, 0, 31,
                         header_size, header_size + len(image), mode)
    return header + bytes(image) + bytes(mask)


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    picture = Image.open(sys.argv[1])
    sprites = []
    for spec in sys.argv[3:]:
        name, size, *rest = spec.split(':')
        width, height = (int(n) for n in size.split('x'))
        sprites.append(sprite(picture, name, width, height, int(rest[0]) if rest else 90))
    body = b''.join(sprites)
    # A sprite file is a sprite area without its first word (the area's size).
    with open(sys.argv[2], 'wb') as out:
        out.write(struct.pack('<3I', len(sprites), 16, 16 + len(body)) + body)


if __name__ == '__main__':
    main()
