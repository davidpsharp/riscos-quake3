/*
===========================================================================
RISC OS: whether OpenGL will be drawn by a GPU, for the defaults that suit it.

riscos-mesa draws in software unless RISC3D has a driver: RISC3D_Info gives
its name (r3 0: none) and flags (r4 bit 1: it draws in software too, as
RoMac's does without a host GPU). As riscos-mesa decides, RISC3D$Disable or
LIBGL_ALWAYS_SOFTWARE set means software. Asked once, before any context:
the render size is chosen before the window is made.

RISC OS port, copyright David Sharp 2026
===========================================================================
*/
#ifndef RISCOS_GPU_H
#define RISCOS_GPU_H

#ifdef __riscos__
#include <kernel.h>

#define RISCOS_RISC3D_Info		0x5AF80
#define RISCOS_RISC3D_DRIVER_SOFTWARE	2

static int RISCOS_HaveGPU( void )
{
	static int known = -1;
	_kernel_swi_regs regs;
	char value[8];

	if ( known >= 0 )
		return known;
	known = 0;
	if ( !_kernel_getenv( "RISC3D$Disable", value, sizeof( value ) ) ||
		!_kernel_getenv( "LIBGL_ALWAYS_SOFTWARE", value, sizeof( value ) ) )
		return known;
	if ( _kernel_swi( RISCOS_RISC3D_Info, &regs, &regs ) )
		return known;	// no RISC3D module
	known = regs.r[3] != 0 && !( regs.r[4] & RISCOS_RISC3D_DRIVER_SOFTWARE );
	return known;
}
#endif

#endif
