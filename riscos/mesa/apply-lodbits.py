#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""riscos-mesa's s_triangle.c, fast_tex_image: the mip level of a triangle
(GL_FASTEST hint) was two sqrtf calls and a log2f, UnixLib C functions,
for every triangle. With no LOD bias the level is
floor(log2(rho) + 0.5) = floor(log2(2 rho^2)) >> 1, which is the float
exponent of 2 rho^2 halved: no libm calls. The same level, bit for bit,
except where rho sits within float rounding of a level boundary.
    apply-lodbits.py path/to/s_triangle.c   (on the original file)"""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''   rho = MAX2(sqrtf(dsdx * dsdx + dtdx * dtdx), sqrtf(dsdy * dsdy + dtdy * dtdy));
   lod = rho > 0.0F ? log2f(rho) : 0.0F;
   lod += samp->LodBias + ctx->Texture.Unit[0].LodBias;

   if (lod <= 0.0F) {
      *filter = samp->MagFilter;
      return base;
   }
'''
new = '''   /* q3-riscos: rho^2, and with no LOD bias the level from its float
    * exponent (level = floor(log2(rho) + 0.5) = floor(log2(2 rho^2)) / 2),
    * instead of two sqrtf and a log2f (UnixLib C functions) a triangle */
   rho = MAX2(dsdx * dsdx + dtdx * dtdx, dsdy * dsdy + dtdy * dtdy);
   if (samp->LodBias + ctx->Texture.Unit[0].LodBias == 0.0F) {
      union { GLfloat f; GLuint u; } r2;
      if (rho <= 1.0F) {
         *filter = samp->MagFilter;
         return base;
      }
      r2.f = 2.0F * rho;
      /* e = floor(log2(2 rho^2)) >= 1: log2(rho) is within [e/2 - 1/2,
       * e/2); e/2 - 1/4 gives the same level and filter below */
      lod = (GLfloat) ((GLint) ((r2.u >> 23) & 0xff) - 127) * 0.5F - 0.25F;
   }
   else {
      rho = sqrtf(rho);
      lod = rho > 0.0F ? log2f(rho) : 0.0F;
      lod += samp->LodBias + ctx->Texture.Unit[0].LodBias;
   }

   if (lod <= 0.0F) {
      *filter = samp->MagFilter;
      return base;
   }
'''
assert s.count(old) == 1, 'fast_tex_image not as expected'
s = s.replace(old, new)
open(p, 'w').write(s)
print('patched', p)
