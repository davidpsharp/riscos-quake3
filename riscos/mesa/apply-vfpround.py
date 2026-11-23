#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""Mesa's src/util/rounding.h: _mesa_lroundevenf (behind IROUND, F_TO_I,
UNCLAMPED_FLOAT_TO_UBYTE: every vertex colour's float to byte conversion
among much else) calls lrintf, which in UnixLib is a C function. With VFP
it is one instruction, vcvtr.s32.f32, which rounds in the current mode
(to nearest even, as lrintf): the same value.
    apply-vfpround.py path/to/src/util/rounding.h   (on the original file)"""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''#else
   return lrintf(x);
#endif
}
'''
new = '''#elif defined(__arm__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
   /* q3-riscos: one VFP instruction (the current rounding mode, to
    * nearest even, as lrintf) instead of UnixLib's C lrintf */
   union { float f; int i; } r;
   __asm__ ("vcvtr.s32.f32 %0, %1" : "=t" (r.f) : "t" (x));
   return r.i;
#else
   return lrintf(x);
#endif
}
'''
assert s.count(old) == 1, 'rounding.h not as expected'
s = s.replace(old, new)
open(p, 'w').write(s)
print('patched', p)
