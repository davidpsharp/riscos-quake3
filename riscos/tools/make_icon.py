#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""Draws the application's icon, 512x512 with transparency: a steel disc
with a sight ring and a glowing "3" made of two arcs (no font, and nothing of
id Software's logos). riscos/tools/make_sprites.py makes the sprites from it.
    python3 -I riscos/tools/make_icon.py build/icon.png"""
import sys

from PIL import Image, ImageDraw, ImageFilter

S = 512
C = S // 2


def disc(draw, r, fill=None, outline=None, width=1):
    draw.ellipse((C - r, C - r, C + r, C + r), fill=fill, outline=outline, width=width)


def main():
    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))

    # The disc: steel, lighter towards the top left, with a bright rim
    steel = Image.new('RGBA', (S, S))
    sd = ImageDraw.Draw(steel)
    for i in range(240, 0, -2):
        t = i / 240
        g = int(40 + 70 * (1 - t))
        sd.ellipse((C - i - 30 * (1 - t), C - i - 30 * (1 - t), C + i - 30 * (1 - t), C + i - 30 * (1 - t)),
                   fill=(g, g + 3, g + 10, 255))
    mask = Image.new('L', (S, S), 0)
    disc(ImageDraw.Draw(mask), 240, fill=255)
    img.paste(steel, (0, 0), mask)
    d = ImageDraw.Draw(img)
    disc(d, 240, outline=(20, 22, 26, 255), width=10)
    disc(d, 230, outline=(190, 196, 206, 255), width=10)
    disc(d, 214, outline=(28, 30, 35, 255), width=6)

    # The sight ring, broken at the four points, with ticks out to the rim
    ring = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    rd = ImageDraw.Draw(ring)
    for a in (0, 90, 180, 270):
        rd.arc((C - 192, C - 192, C + 192, C + 192), a + 12, a + 78, fill=(255, 120, 30, 255), width=14)
    for x0, y0, x1, y1 in ((C - 7, 26, C + 7, 92), (C - 7, S - 92, C + 7, S - 26),
                           (26, C - 7, 92, C + 7), (S - 92, C - 7, S - 26, C + 7)):
        rd.rectangle((x0, y0, x1, y1), fill=(255, 120, 30, 255))

    # The "3": two arcs, open to the left, meeting in the middle
    three = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    td = ImageDraw.Draw(three)
    w = 46
    td.arc((C - 92, 120, C + 84, 266), 200, 90, fill=(255, 255, 255, 255), width=w)
    td.arc((C - 100, 246, C + 92, 402), 270, 160, fill=(255, 255, 255, 255), width=w)
    # Colour it: yellow at the top to red at the bottom
    grad = Image.new('RGBA', (S, S))
    gd = ImageDraw.Draw(grad)
    for y in range(S):
        t = min(max((y - 120) / 280, 0), 1)
        gd.line((0, y, S, y), fill=(255, int(220 - 170 * t), int(60 - 50 * t), 255))
    shape = three.split()[3]
    coloured = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    coloured.paste(grad, (0, 0), shape)

    # Glow behind the ring and the "3"
    glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    glow.paste((255, 60, 0, 200), (0, 0), shape.filter(ImageFilter.MaxFilter(17)))
    glow.paste((255, 80, 0, 120), (0, 0), ring.split()[3].filter(ImageFilter.MaxFilter(9)))
    glow = glow.filter(ImageFilter.GaussianBlur(14))
    inside = Image.new('L', (S, S), 0)
    disc(ImageDraw.Draw(inside), 212, fill=255)
    clipped = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    clipped.paste(glow, (0, 0), Image.composite(glow.split()[3], Image.new('L', (S, S), 0), inside))
    img = Image.alpha_composite(img, clipped)
    img = Image.alpha_composite(img, ring)
    # A dark edge round the "3" so it holds up when small
    edge = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    edge.paste((30, 8, 0, 255), (0, 0), shape.filter(ImageFilter.MaxFilter(11)))
    img = Image.alpha_composite(img, edge)
    img = Image.alpha_composite(img, coloured)
    img.save(sys.argv[1])


if __name__ == '__main__':
    main()
