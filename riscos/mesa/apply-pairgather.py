#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026
"""Adds the pair gather to riscos-mesa's s_triangle.c (ro_tex_group, BGR):
for groups whose four pixels each have their two texels of a row side by
side (s1 == s0 + 1: nothing wrapped or clamped between them), each row's
pair is loaded with one 8-byte vld1 (NEON byte loads need no alignment)
and the channels picked out with table lookups, instead of sixteen
dependent vld3 lane loads. Same texels, so the same result bit for bit.
    apply-pairgather.py path/to/s_triangle.c"""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''   else {                                      /* BGR: bytes B G R */
      const GLchan *t8 = tx->texture;
      uint8x8x3_t pa, pb;
      pa.val[0] = pa.val[1] = pa.val[2] = vdup_n_u8(0);'''
new = '''   else if (ro_pair_gather &&
            ro_all_one(vsubq_s32(s1, s0)) &&
            ((tx->tmask + 1) << tx->twidth_log2) >= 4) {
      /* q3-riscos pair gather: each pixel's s0 and s1 texels of a row
       * are side by side. One 8-byte load per pixel and row, starting two
       * bytes before the s0 texel (at the texture's start, at it), so it
       * never reads past the s1 texel's row end or the texture's end;
       * the channels are picked out with table lookups. */
      static const uint8_t hi4[8] = { 0, 0, 0, 0, 255, 255, 255, 255 };
      static const uint8_t lo4[8] = { 255, 255, 255, 255, 0, 0, 0, 0 };
      const GLchan *t8 = tx->texture;
      const uint8x8_t hi = vld1_u8(hi4), lo = vld1_u8(lo4);
      uint8x8x4_t q0, q1;      /* the pixels' row t0 and row t1 bytes */
      uint8_t ib[8];
      uint8x8_t base;
      GLuint k;
      for (k = 0; k < 4; k++) {
         const GLint n0 = o[k], n1 = o[4 + k];
         const GLint b0 = n0 > 0 ? 2 : 0, b1 = n1 > 0 ? 2 : 0;
         q0.val[k] = vld1_u8(t8 + 3 * n0 - b0);
         q1.val[k] = vld1_u8(t8 + 3 * n1 - b1);
         ib[k] = (uint8_t) (8 * k + b0);
         ib[4 + k] = (uint8_t) (8 * k + b1);
      }
      base = vld1_u8(ib);
      for (c = 0; c < 3; c++) {
         const uint8x8_t i0 = vadd_u8(base, vdup_n_u8((uint8_t) (2 - c)));
         const uint8x8_t i1 = vadd_u8(i0, vdup_n_u8(3));
         const uint8x8_t a = vtbx4_u8(vtbl4_u8(q0, vorr_u8(i0, hi)), q1, vorr_u8(i0, lo));
         const uint8x8_t b = vtbx4_u8(vtbl4_u8(q0, vorr_u8(i1, hi)), q1, vorr_u8(i1, lo));
         v[c] = ro_bilerp4(a, b, sf, tf);
      }
      v[3] = vdup_n_s16(CHAN_MAX);             /* LINEAR_RGB's alpha */
   }
   else {                                      /* BGR: bytes B G R */
      const GLchan *t8 = tx->texture;
      uint8x8x3_t pa, pb;
      pa.val[0] = pa.val[1] = pa.val[2] = vdup_n_u8(0);'''
assert old in s, "ro_tex_group's BGR branch not found"
s = s.replace(old, new, 1)
# helpers before ro_tex_group
anchor = '''/* four pixels: texel coordinates sfix, tfix (less FIXED_HALF); colours
 * from *cols (advanced past the four). Inlined into loops made for each
 * format and mode (constants there). */'''
assert anchor in s
helpers = '''/* q3-riscos: every lane 1 */
static inline GLboolean
ro_all_one(int32x4_t d)
{
   const uint32x4_t e = vceqq_s32(d, vdupq_n_s32(1));
   uint32x2_t m = vand_u32(vget_low_u32(e), vget_high_u32(e));
   m = vpmin_u32(m, m);
   return vget_lane_u32(m, 0) != 0;
}

/* q3-riscos: the pair gather, unless MESA_NO_PAIRGATHER is set */
static int ro_pair_gather = 1;

'''
s = s.replace(anchor, helpers + anchor, 1)
# initialise from env where row gather is
old2 = '''   if (ro_row_gather < 0)
      ro_row_gather = !getenv("MESA_NO_ROWGATHER");'''
assert old2 in s
s = s.replace(old2, old2 + '''
   if (ro_pair_gather == 1 && getenv("MESA_NO_PAIRGATHER"))
      ro_pair_gather = 0;''', 1)
open(p, 'w').write(s)
print("pair gather added")
