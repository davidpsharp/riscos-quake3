#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""riscos-mesa's s_triangle.c, ro_tex_group's BGR gather: the sixteen vld3
lane loads went into two register sets, eight dependent loads each. Here
they go into four (lanes 0-3 and 4-7 apart, the other lanes zero) that
are then ORed together: chains of four instead of eight. Same texels, same
result bit for bit.
    apply-splitchain.py path/to/s_triangle.c   (on the original file)"""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''      uint8x8x3_t pa, pb;
      pa.val[0] = pa.val[1] = pa.val[2] = vdup_n_u8(0);
      pb = pa;
      RO_LD3(pa, t8 + 3 * o[0], 0);  RO_LD3(pa, t8 + 3 * o[1], 1);
      RO_LD3(pa, t8 + 3 * o[2], 2);  RO_LD3(pa, t8 + 3 * o[3], 3);
      RO_LD3(pa, t8 + 3 * o[4], 4);  RO_LD3(pa, t8 + 3 * o[5], 5);
      RO_LD3(pa, t8 + 3 * o[6], 6);  RO_LD3(pa, t8 + 3 * o[7], 7);
      RO_LD3(pb, t8 + 3 * o[8], 0);  RO_LD3(pb, t8 + 3 * o[9], 1);
      RO_LD3(pb, t8 + 3 * o[10], 2); RO_LD3(pb, t8 + 3 * o[11], 3);
      RO_LD3(pb, t8 + 3 * o[12], 4); RO_LD3(pb, t8 + 3 * o[13], 5);
      RO_LD3(pb, t8 + 3 * o[14], 6); RO_LD3(pb, t8 + 3 * o[15], 7);
      for (c = 0; c < 3; c++)
         v[c] = ro_bilerp4(pa.val[2 - c], pb.val[2 - c], sf, tf);'''
new = '''      /* q3-riscos: four independent chains of four lane loads (lanes
       * 0-3 and 4-7 of each set apart, the rest zero), ORed together */
      uint8x8x3_t pa, pb, pa2, pb2;
      pa.val[0] = pa.val[1] = pa.val[2] = vdup_n_u8(0);
      pb = pa2 = pb2 = pa;
      RO_LD3(pa, t8 + 3 * o[0], 0);   RO_LD3(pa2, t8 + 3 * o[4], 4);
      RO_LD3(pb, t8 + 3 * o[8], 0);   RO_LD3(pb2, t8 + 3 * o[12], 4);
      RO_LD3(pa, t8 + 3 * o[1], 1);   RO_LD3(pa2, t8 + 3 * o[5], 5);
      RO_LD3(pb, t8 + 3 * o[9], 1);   RO_LD3(pb2, t8 + 3 * o[13], 5);
      RO_LD3(pa, t8 + 3 * o[2], 2);   RO_LD3(pa2, t8 + 3 * o[6], 6);
      RO_LD3(pb, t8 + 3 * o[10], 2);  RO_LD3(pb2, t8 + 3 * o[14], 6);
      RO_LD3(pa, t8 + 3 * o[3], 3);   RO_LD3(pa2, t8 + 3 * o[7], 7);
      RO_LD3(pb, t8 + 3 * o[11], 3);  RO_LD3(pb2, t8 + 3 * o[15], 7);
      for (c = 0; c < 3; c++)
         v[c] = ro_bilerp4(vorr_u8(pa.val[2 - c], pa2.val[2 - c]),
                           vorr_u8(pb.val[2 - c], pb2.val[2 - c]), sf, tf);'''
assert old in s, "BGR gather not found"
s = s.replace(old, new, 1)
open(p, 'w').write(s)
print("split chains added")
