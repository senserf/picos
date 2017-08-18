/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	__encrypt_c__
// This is for SMURPH inclusion from source library
#define	__encrypt_c__

#ifdef __SMURPH__
#include "board.h"
#else
#include "encrypt.h"
#endif

/*
 * TEA encryption/decryption
 */

#define	zcr_a 		key [0]
#define	zcr_b 		key [1]
#define	zcr_c 		key [2]
#define	zcr_d 		key [3]
#define zcr_delta 	0x9E3779B9
#define	zcr_gamma 	0xC6EF3720

void encrypt (word *str, int nw, const lword *key) {
/*
 * TEA encryption of the string of words in place using a 128-bit key.
 * The string is encrypted from the end, which assumes that the IV is
 * located at the end of the string.
 */

	int 	n;
	lword	y, z, sum;

	y = z = 0L;

	// Full blocks first
	while (nw >= 4) {
		y ^= ((lword) str [nw-4] << 16) | str [nw - 3];
		z ^= ((lword) str [nw-2] << 16) | str [nw - 1];
		sum = 0L;
		for (n = 0; n < 32; n++) {
			sum += zcr_delta;
      			y += ((z << 4) + zcr_a) ^ (z + sum) ^ ((z >> 5) + zcr_b);
      			z += ((y << 4) + zcr_c) ^ (y + sum) ^ ((y >> 5) + zcr_d);
		}
		str [nw-4] = (word) (y >> 16);
		str [nw-3] = (word)  y;
		str [nw-2] = (word) (z >> 16);
		str [nw-1] = (word)  z;
		nw -= 4;
	}

	if (nw == 0)
		return;

	// One more iteration to encrypt the partial block
	sum = 0L;
	for (n = 0; n < 32; n++) {
		sum += zcr_delta;
      		y += ((z << 4) + zcr_a) ^ (z + sum) ^ ((z >> 5) + zcr_b);
      		z += ((y << 4) + zcr_c) ^ (y + sum) ^ ((y >> 5) + zcr_d);
	}

	nw--;
	str [nw] ^= (word) z;

	if (nw == 0)
		return;

	nw--;
	str [nw] ^= (word) (z >> 16);

	if (nw == 0)
		return;

	nw--;
	str [nw] ^= (word) y;
}

void decrypt (word *str, int nw, const lword *key) {

	int	n;
	lword	y, z, sum;
	word	yh, yl, zh, zl;

#define	xc	(*((word*)(&sum)))

	yh = yl = zh = zl = 0;

	// Full blocks first
	while (nw >= 4) {
		y = ((lword) str [nw-4] << 16) | str [nw - 3];
		z = ((lword) str [nw-2] << 16) | str [nw - 1];
		sum = zcr_gamma;
		for (n = 0; n < 32; n++) {
			z -= ((y << 4) + zcr_c) ^ (y + sum) ^ ((y >> 5) + zcr_d);
      			y -= ((z << 4) + zcr_a) ^ (z + sum) ^ ((z >> 5) + zcr_b);
      			sum -= zcr_delta;
      		}
		xc = yh;
		yh = str [nw-4];
		str [nw-4] = ((word) (y >> 16)) ^ xc;
		xc = yl;
		yl = str [nw-3];
		str [nw-3] = ((word)  y       ) ^ xc;
		xc = zh;
		zh = str [nw-2];
		str [nw-2] = ((word) (z >> 16)) ^ xc;
		xc = zl;
		zl = str [nw-1];
		str [nw-1] = ((word)  z       ) ^ xc;
		nw -= 4;
	}

	if (nw == 0)
		return;

	// Encrypt the last encrypted block to recover the tail

	sum = 0L;
	y = ((lword) yh << 16) | yl;
	z = ((lword) zh << 16) | zl;
	for (n = 0; n < 32; n++) {
		sum += zcr_delta;
      		y += ((z << 4) + zcr_a) ^ (z + sum) ^ ((z >> 5) + zcr_b);
      		z += ((y << 4) + zcr_c) ^ (y + sum) ^ ((y >> 5) + zcr_d);
	}

	nw--;
	str [nw] ^= (word) z;

	if (nw == 0)
		return;

	nw--;
	str [nw] ^= (word) (z >> 16);

	if (nw == 0)
		return;

	nw--;
	str [nw] ^= (word) y;
}

#undef	zcr_a
#undef	zcr_b
#undef	zcr_c
#undef	zcr_d
#undef 	zcr_delta
#undef	zcr_gamma

#endif
