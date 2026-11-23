/* SHA-256 (FIPS 180-4), small and incremental, for checking downloads. */
/* RISC OS port, copyright David Sharp 2026, GPL 2 */
#ifndef LAUNCHER_SHA256_H
#define LAUNCHER_SHA256_H
#include <stddef.h>
#include <stdint.h>

struct Sha256 {
	uint32_t h[8];
	uint64_t length;	// bytes so far
	unsigned char block[64];
	size_t used;		// bytes in block
};

void Sha256_Init(Sha256 & s);
void Sha256_Update(Sha256 & s, void const * data, size_t n);
// Finishes and writes the digest as 64 lower-case hex digits and a terminator.
void Sha256_Hex(Sha256 & s, char out[65]);

#endif
