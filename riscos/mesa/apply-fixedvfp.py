#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026
"""riscos-mesa's s_context.h, after apply-fixedsetup.py: ro_float_to_fixed
as one VFP conversion, vcvtr.s32.f32 (the current rounding mode, to nearest
even), instead of the truncate-and-fix-up C code. Ties (an exact .5) round
to even instead of away from zero, which lroundf did: a different pixel at
a triangle's edge now and then, nothing else. An experiment: only if it
measures faster than apply-fixedsetup.py alone.
    apply-fixedvfp.py path/to/src/mesa/swrast/s_context.h"""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''static inline long
ro_float_to_fixed(float x)
{
   const float y = x * FIXED_SCALE;
   if (y > -2147483648.0f && y < 2147483648.0f) {
      long t = (long) (int) y;
      const float f = y - (float) t;
      if (f >= 0.5f)
         t++;
      else if (f <= -0.5f)
         t--;
      return t;
   }
   return lroundf(y);
}'''
new = '''static inline long
ro_float_to_fixed(float x)
{
   /* q3-riscos: one VFP conversion (to nearest even; out of range values
    * saturate, as vcvtr does) */
   union { float f; int i; } r;
   __asm__ ("vcvtr.s32.f32 %0, %1" : "=t" (r.f) : "t" (x * FIXED_SCALE));
   return r.i;
}'''
assert s.count(old) == 1, 's_context.h not as expected'
open(p, 'w').write(s.replace(old, new))
print('patched', p)
