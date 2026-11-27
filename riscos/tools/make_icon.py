#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""Draws the application's icon, 512x512 with transparency: three dark red stakes,
thick at the top and pointed at the bottom, the outer two leaning in, with a soft
orange glow behind (nothing of id Software's logos). riscos/tools/make_sprites.py
makes the sprites from it, with alpha masks so the edges and the glow blend.
    python3 riscos/tools/make_icon.py riscos/app-icon.png"""
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

S = 512
C = S // 2


def vgrad(stops):
    """A vertical gradient through (y, (r, g, b)) stops."""
    g = Image.new('RGBA', (S, S))
    d = ImageDraw.Draw(g)
    for y in range(S):
        for (y0, c0), (y1, c1) in zip(stops, stops[1:]):
            if y0 <= y <= y1:
                t = (y - y0) / max(y1 - y0, 1)
                c = tuple(int(a + (b - a) * t) for a, b in zip(c0, c1))
                break
        else:
            c = stops[0][1] if y < stops[0][0] else stops[-1][1]
        d.line((0, y, S, y), fill=c + (255,))
    return g


def main():
    # The stakes: (centre at the top, centre at the point, top, point, width at the top)
    m = Image.new('L', (S, S), 0)
    d = ImageDraw.Draw(m)
    for top_cx, tip_cx, top, tip, w in ((C - 140, C - 66, 96, 420, 88), (C, C, 72, 452, 100),
                                        (C + 140, C + 66, 96, 420, 88)):
        d.polygon([(top_cx - w / 2, top), (top_cx + w / 2, top), (tip_cx, tip)], fill=255)

    # The glow behind
    glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    glow.paste((255, 160, 60, 170), (0, 0), m.filter(ImageFilter.MaxFilter(31)))
    img = glow.filter(ImageFilter.GaussianBlur(18))

    # A near-black outline, then the stakes: red at the top to almost black at the points
    outline = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    outline.paste((10, 0, 0, 255), (0, 0), m.filter(ImageFilter.MaxFilter(11)))
    img = Image.alpha_composite(img, outline)
    body = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    body.paste(vgrad([(40, (200, 30, 30)), (300, (90, 0, 0)), (470, (30, 0, 0))]), (0, 0), m)
    img = Image.alpha_composite(img, body)

    # Bevel: light on the upper left edges, dark on the lower right
    lit = ImageChops.subtract(m, ImageChops.offset(m, 6, 6))
    dark = ImageChops.subtract(m, ImageChops.offset(m, -6, -6))
    bevel = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    bevel.paste((255, 200, 190, 150), (0, 0), lit)
    bevel.paste((40, 0, 0, 170), (0, 0), dark)
    img = Image.alpha_composite(img, bevel)
    img.save(sys.argv[1])


if __name__ == '__main__':
    main()
