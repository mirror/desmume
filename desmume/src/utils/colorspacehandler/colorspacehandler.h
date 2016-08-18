/*
	Copyright (C) 2016 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COLORSPACEHANDLER_H
#define COLORSPACEHANDLER_H

#include "types.h"
#include <stdio.h>
#include <stdint.h>


enum NDSColorFormat
{
	// The color format information is packed in a 32-bit value.
	// The bits are as follows:
	// FFFOOOOO AAAAAABB BBBBGGGG GGRRRRRR
	//
	// F = Flags (see below)
	// O = Color order (see below)
	// A = Bit count for alpha [0-63]
	// B = Bit count for blue  [0-63]
	// G = Bit count for green [0-63]
	// R = Bit count for red   [0-63]
	//
	// Flags:
	//   Bit 29: Reverse order flag.
	//      Set = Bits are in reverse order, usually for little-endian usage.
	//      Cleared = Bits are in normal order, usually for big-endian usage.
	//
	// Color order bits, 24-28:
	//      0x00 = RGBA, common format
	//      0x01 = RGAB
	//      0x02 = RBGA
	//      0x03 = RBAG
	//      0x04 = RAGB
	//      0x05 = RABG
	//      0x06 = GRBA
	//      0x07 = GRAB
	//      0x08 = GBRA
	//      0x09 = GBAR
	//      0x0A = GARB
	//      0x0B = GABR
	//      0x0C = BRGA
	//      0x0D = BRAG
	//      0x0E = BGRA, common format
	//      0x0F = BGAR
	//      0x10 = BARG
	//      0x11 = BAGR
	//      0x12 = ARGB
	//      0x13 = ARBG
	//      0x14 = AGRB
	//      0x15 = AGBR
	//      0x16 = ABRG
	//      0x17 = ABGR
	
	// Color formats used for internal processing.
	//NDSColorFormat_ABGR1555_Rev		= 0x20045145,
	//NDSColorFormat_ABGR5666_Rev		= 0x20186186,
	//NDSColorFormat_ABGR8888_Rev		= 0x20208208,
	
	// Color formats used by the output framebuffers.
	NDSColorFormat_BGR555_Rev		= 0x20005145,
	NDSColorFormat_BGR666_Rev		= 0x20006186,
	NDSColorFormat_BGR888_Rev		= 0x20008208
};

union FragmentColor
{
	u32 color;
	struct
	{
		u8 r,g,b,a;
	};
};

extern CACHE_ALIGN const u32 material_5bit_to_31bit[32];
extern CACHE_ALIGN const u8 material_5bit_to_6bit[32];
extern CACHE_ALIGN const u8 material_5bit_to_8bit[32];
extern CACHE_ALIGN const u8 material_6bit_to_8bit[64];
extern CACHE_ALIGN const u8 material_3bit_to_5bit[8];
extern CACHE_ALIGN const u8 material_3bit_to_6bit[8];
extern CACHE_ALIGN const u8 material_3bit_to_8bit[8];

extern CACHE_ALIGN u32 color_555_to_6665_opaque[32768];
extern CACHE_ALIGN u32 color_555_to_6665_opaque_swap_rb[32768];
extern CACHE_ALIGN u32 color_555_to_666[32768];
extern CACHE_ALIGN u32 color_555_to_8888_opaque[32768];
extern CACHE_ALIGN u32 color_555_to_8888_opaque_swap_rb[32768];
extern CACHE_ALIGN u32 color_555_to_888[32768];

#define COLOR555TO6665_OPAQUE(col) (color_555_to_6665_opaque[(col)])					// Convert a 15-bit color to an opaque sparsely packed 32-bit color containing an RGBA6665 color
#define COLOR555TO6665_OPAQUE_SWAP_RB(col) (color_555_to_6665_opaque_swap_rb[(col)])	// Convert a 15-bit color to an opaque sparsely packed 32-bit color containing an RGBA6665 color with R and B components swapped
#define COLOR555TO666(col) (color_555_to_666[(col)])									// Convert a 15-bit color to a fully transparent sparsely packed 32-bit color containing an RGBA6665 color

#ifdef LOCAL_LE
	#define COLOR555TO6665(col,alpha5) (((alpha5)<<24) | color_555_to_666[(col)])		// Convert a 15-bit color to a sparsely packed 32-bit color containing an RGBA6665 color with user-defined alpha, little-endian
#else
	#define COLOR555TO6665(col,alpha5) ((alpha5) | color_555_to_666[(col)])				// Convert a 15-bit color to a sparsely packed 32-bit color containing an RGBA6665 color with user-defined alpha, big-endian
#endif

#define COLOR555TO8888_OPAQUE(col) (color_555_to_8888_opaque[(col)])					// Convert a 15-bit color to an opaque 32-bit color
#define COLOR555TO8888_OPAQUE_SWAP_RB(col) (color_555_to_8888_opaque_swap_rb[(col)])	// Convert a 15-bit color to an opaque 32-bit color with R and B components swapped
#define COLOR555TO888(col) (color_555_to_888[(col)])									// Convert a 15-bit color to an opaque 24-bit color or a fully transparent 32-bit color

#ifdef LOCAL_LE
	#define COLOR555TO8888(col,alpha8) (((alpha8)<<24) | color_555_to_888[(col)])		// Convert a 15-bit color to a 32-bit color with user-defined alpha, little-endian
#else
	#define COLOR555TO8888(col,alpha8) ((alpha8) | color_555_to_888[(col)])				// Convert a 15-bit color to a 32-bit color with user-defined alpha, big-endian
#endif

//produce a 15bpp color from individual 5bit components
#define R5G5B5TORGB15(r,g,b) ( (r) | ((g)<<5) | ((b)<<10) )

//produce a 16bpp color from individual 5bit components
#define R6G6B6TORGB15(r,g,b) ( ((r)>>1) | (((g)&0x3E)<<4) | (((b)&0x3E)<<9) )

void ColorspaceHandlerInit();

template <bool SWAP_RB>
FORCEINLINE u32 ColorspaceConvert555To8888Opaque(const u16 src)
{
	return (SWAP_RB) ? COLOR555TO8888_OPAQUE_SWAP_RB(src & 0x7FFF) : COLOR555TO8888_OPAQUE(src & 0x7FFF);
}

template <bool SWAP_RB>
FORCEINLINE u32 ColorspaceConvert555To6665Opaque(const u16 src)
{
	return (SWAP_RB) ? COLOR555TO6665_OPAQUE_SWAP_RB(src & 0x7FFF) : COLOR555TO6665_OPAQUE(src & 0x7FFF);
}

template <bool SWAP_RB>
FORCEINLINE u32 ColorspaceConvert8888To6665(FragmentColor srcColor)
{
	FragmentColor outColor;
	outColor.r = ((SWAP_RB) ? srcColor.b : srcColor.r) >> 2;
	outColor.g = srcColor.g >> 2;
	outColor.b = ((SWAP_RB) ? srcColor.r : srcColor.b) >> 2;
	outColor.a = srcColor.a >> 3;
	
	return outColor.color;
}

template <bool SWAP_RB>
FORCEINLINE u32 ColorspaceConvert8888To6665(u32 srcColor)
{
	FragmentColor srcColorComponent;
	srcColorComponent.color = srcColor;
	
	return ColorspaceConvert8888To6665<SWAP_RB>(srcColorComponent);
}

template <bool SWAP_RB>
FORCEINLINE u32 ColorspaceConvert6665To8888(FragmentColor srcColor)
{
	FragmentColor outColor;
	outColor.r = material_6bit_to_8bit[((SWAP_RB) ? srcColor.b : srcColor.r)];
	outColor.g = material_6bit_to_8bit[srcColor.g];
	outColor.b = material_6bit_to_8bit[((SWAP_RB) ? srcColor.r : srcColor.b)];
	outColor.a = material_5bit_to_8bit[srcColor.a];
	
	return outColor.color;
}

template <bool SWAP_RB>
FORCEINLINE u32 ColorspaceConvert6665To8888(u32 srcColor)
{
	FragmentColor srcColorComponent;
	srcColorComponent.color = srcColor;
	
	return ColorspaceConvert6665To8888<SWAP_RB>(srcColorComponent);
}

template <bool SWAP_RB>
FORCEINLINE u16 ColorspaceConvert8888To5551(FragmentColor srcColor)
{
	return R5G5B5TORGB15( ((SWAP_RB) ? srcColor.b : srcColor.r) >> 3, srcColor.g >> 3, ((SWAP_RB) ? srcColor.r : srcColor.b) >> 3) | ((srcColor.a == 0) ? 0x0000 : 0x8000 );
}

template <bool SWAP_RB>
FORCEINLINE u16 ColorspaceConvert8888To5551(u32 srcColor)
{
	FragmentColor srcColorComponent;
	srcColorComponent.color = srcColor;
	
	return ColorspaceConvert8888To5551<SWAP_RB>(srcColorComponent);
}

template <bool SWAP_RB>
FORCEINLINE u16 ColorspaceConvert6665To5551(FragmentColor srcColor)
{
	return R6G6B6TORGB15( ((SWAP_RB) ? srcColor.b : srcColor.r), srcColor.g, ((SWAP_RB) ? srcColor.r : srcColor.b)) | ((srcColor.a == 0) ? 0x0000 : 0x8000);
}

template <bool SWAP_RB>
FORCEINLINE u16 ColorspaceConvert6665To5551(u32 srcColor)
{
	FragmentColor srcColorComponent;
	srcColorComponent.color = srcColor;
	
	return ColorspaceConvert6665To5551<SWAP_RB>(srcColorComponent);
}

template<bool SWAP_RB, bool IS_UNALIGNED> void ColorspaceConvertBuffer555To8888Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template<bool SWAP_RB, bool IS_UNALIGNED> void ColorspaceConvertBuffer555To6665Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template<bool SWAP_RB, bool IS_UNALIGNED> void ColorspaceConvertBuffer8888To6665(const u32 *src, u32 *dst, size_t pixCount);
template<bool SWAP_RB, bool IS_UNALIGNED> void ColorspaceConvertBuffer6665To8888(const u32 *src, u32 *dst, size_t pixCount);
template<bool SWAP_RB, bool IS_UNALIGNED> void ColorspaceConvertBuffer8888To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template<bool SWAP_RB, bool IS_UNALIGNED> void ColorspaceConvertBuffer6665To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);

class ColorspaceHandler
{
public:
	ColorspaceHandler() {};
	
	size_t ConvertBuffer555To8888Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer555To8888Opaque_SwapRB(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer555To8888Opaque_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer555To8888Opaque_SwapRB_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	
	size_t ConvertBuffer555To6665Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer555To6665Opaque_SwapRB(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer555To6665Opaque_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer555To6665Opaque_SwapRB_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const;
	
	size_t ConvertBuffer8888To6665(const u32 *src, u32 *dst, size_t pixCount) const;
	size_t ConvertBuffer8888To6665_SwapRB(const u32 *src, u32 *dst, size_t pixCount) const;
	size_t ConvertBuffer8888To6665_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const;
	size_t ConvertBuffer8888To6665_SwapRB_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const;
	
	size_t ConvertBuffer6665To8888(const u32 *src, u32 *dst, size_t pixCount) const;
	size_t ConvertBuffer6665To8888_SwapRB(const u32 *src, u32 *dst, size_t pixCount) const;
	size_t ConvertBuffer6665To8888_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const;
	size_t ConvertBuffer6665To8888_SwapRB_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const;
	
	size_t ConvertBuffer8888To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer8888To5551_SwapRB(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer8888To5551_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer8888To5551_SwapRB_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	
	size_t ConvertBuffer6665To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer6665To5551_SwapRB(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer6665To5551_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
	size_t ConvertBuffer6665To5551_SwapRB_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const;
};

FORCEINLINE FragmentColor MakeFragmentColor(const u8 r, const u8 g, const u8 b, const u8 a)
{
	FragmentColor ret;
	ret.r = r; ret.g = g; ret.b = b; ret.a = a;
	return ret;
}

#endif /* COLORSPACEHANDLER_H */
