/*  Copyright (C) 2009 DeSmuME team

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "filter.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

typedef u64 uint64;

extern CACHE_ALIGN u16 fadeOutColors[17][0x8000];

extern int scanline_filter_a, scanline_filter_b;
static int fac_a, fac_b;

FORCEINLINE void ScanLine16( uint16 *lpDst, uint16 *lpSrc, unsigned int Width){
	while(Width--){
		*lpDst++ = *lpSrc;
		*lpDst++ = fadeOutColors[scanline_filter_a][(*lpSrc++)];
	}
}

FORCEINLINE void ScanLine16_2( uint16 *lpDst, uint16 *lpSrc, unsigned int Width){
	while(Width--){
		*lpDst++ = fadeOutColors[scanline_filter_a][(*lpSrc)];
		*lpDst++ = fadeOutColors[scanline_filter_b][(*lpSrc++)];
	}
}

FORCEINLINE void ScanLine32( uint32 *lpDst, uint32 *lpSrc, unsigned int Width){
	while(Width--){
		*lpDst++ = *lpSrc;
		u8* u8dst = (u8*)lpDst;
		u8* u8src = (u8*)lpSrc;
		*u8dst++ = *u8src++ * fac_a / 16;
		*u8dst++ = *u8src++ * fac_a / 16;
		*u8dst++ = *u8src++ * fac_a / 16;
		lpDst++; 
		lpSrc++;
	}
}

FORCEINLINE void ScanLine32_2( uint32 *lpDst, uint32 *lpSrc, unsigned int Width){
	while(Width--){
		u8* u8dst = (u8*)lpDst;
		u8* u8src = (u8*)lpSrc;
		*u8dst++ = *u8src++ * fac_a / 16;
		*u8dst++ = *u8src++ * fac_a / 16;
		*u8dst++ = *u8src++ * fac_a / 16;
		u8dst++;
		u8src = (u8*)lpSrc;
		*u8dst++ = *u8src++ * fac_b / 16;
		*u8dst++ = *u8src++ * fac_b / 16;
		*u8dst++ = *u8src++ * fac_b / 16;
		u8dst++; u8src++;
		lpDst+=2; 
		lpSrc++;
	}
}


FORCEINLINE void DoubleLine32( uint32 *lpDst, uint32 *lpSrc, unsigned int Width){
	while(Width--){
		*lpDst++ = *lpSrc;
		*lpDst++ = *lpSrc++;
	}
}

void RenderScanline( SSurface Src, SSurface Dst)
{
	fac_a = (16-scanline_filter_a);
	fac_b = (16-scanline_filter_b);
	unsigned int H;

	const uint32 srcHeight = Src.Height;

	const unsigned int srcPitch = Src.Pitch >> 1;
	u32* lpSrc = (u32*)Src.Surface;

	const unsigned int dstPitch = Dst.Pitch >> 1;
	u32 *lpDst = (u32*)Dst.Surface;
	for (H = 0; H < srcHeight; H++, lpSrc += srcPitch)
		ScanLine32 (lpDst, lpSrc, Src.Width), lpDst += dstPitch,
		ScanLine32_2 (lpDst, lpSrc, Src.Width), lpDst += dstPitch;
		//memset (lpDst, 0, 512*2), lpDst += dstPitch;
}

void RenderNearest2X (SSurface Src, SSurface Dst)
{
	uint32 *lpSrc;
	unsigned int H;

	const uint32 srcHeight = Src.Height;

	const unsigned int srcPitch = Src.Pitch >> 1;
	lpSrc = reinterpret_cast<uint32 *>(Src.Surface);

	const unsigned int dstPitch = Dst.Pitch >> 1;
	uint32 *lpDst = (uint32*)Dst.Surface;
	for (H = 0; H < srcHeight; H++, lpSrc += srcPitch)
		DoubleLine32 (lpDst, lpSrc, Src.Width), lpDst += dstPitch,
		DoubleLine32 (lpDst, lpSrc, Src.Width), lpDst += dstPitch;
}
