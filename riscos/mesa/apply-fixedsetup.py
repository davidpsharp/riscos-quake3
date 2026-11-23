#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""riscos-mesa's s_context.h: FloatToFixed, which the triangle setup
(s_tritemp.h) calls some twenty times a triangle for the edges' and
attributes' fixed point starts and steps, is lroundf: a C function in
UnixLib, 2% of a Quake III frame on a Pi 4 (100,000 calls). riscos-mesa's
ro_float_to_fixed gives the same value inline (it uses it for the spans
only); here the triangle setup uses it too.
    apply-fixedsetup.py path/to/src/mesa/swrast/s_context.h   (on the original file)"""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''   return lroundf(y);
}
#endif
#define FixedToDouble(X) ((X) * (1.0 / FIXED_DBL_SCALE))'''
new = '''   return lroundf(y);
}
/* q3-riscos: the triangle setup's FloatToFixed too (Quake III: 2% of a
 * Pi 4 frame was lroundf, about 100,000 calls a frame) */
#undef FloatToFixed
#define FloatToFixed(X) ro_float_to_fixed(X)
#endif
#define FixedToDouble(X) ((X) * (1.0 / FIXED_DBL_SCALE))'''
assert s.count(old) == 1, 's_context.h not as expected'
open(p, 'w').write(s.replace(old, new))
print('patched', p)
