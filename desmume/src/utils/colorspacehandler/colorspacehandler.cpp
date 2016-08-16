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

#include "colorspacehandler.h"

#if defined(ENABLE_AVX2)
	#include "colorspacehandler_AVX2.h"
#elif defined(ENABLE_SSE2)
	#include "colorspacehandler_SSE2.h"
#elif defined(ENABLE_ALTIVEC)
	#include "colorspacehandler_AltiVec.h"
#endif

#if defined(ENABLE_SSE2) || defined(ENABLE_ALTIVEC)
	#define USEVECTORSIZE_128
#endif

#if defined(ENABLE_AVX2)
	#define USEVECTORSIZE_256
#endif

// By default, the hand-coded vectorized code will be used instead of a compiler's built-in
// autovectorization (if supported). However, if USEMANUALVECTORIZATION is not defined, then
// the compiler will use autovectorization (if supported).
#if defined(USEVECTORSIZE_128) || defined(USEVECTORSIZE_256) || defined(USEVECTORSIZE_512)
	// Comment out USEMANUALVECTORIZATION to disable the hand-coded vectorized code.
	#define USEMANUALVECTORIZATION
#endif

#ifdef USEMANUALVECTORIZATION
	#if defined(ENABLE_AVX2)
	static const ColorspaceHandler_AVX2 csh;
	#elif defined(ENABLE_SSE2)
	static const ColorspaceHandler_SSE2 csh;
	#elif defined(ENABLE_ALTIVEC)
	static const ColorspaceHandler_AltiVec csh;
	#else
	static const ColorspaceHandler csh;
	#endif
#else
	static const ColorspaceHandler csh;
#endif

CACHE_ALIGN u32 color_555_to_6665_opaque[32768];
CACHE_ALIGN u32 color_555_to_6665_opaque_swap_rb[32768];
CACHE_ALIGN u32 color_555_to_666[32768];
CACHE_ALIGN u32 color_555_to_8888_opaque[32768];
CACHE_ALIGN u32 color_555_to_8888_opaque_swap_rb[32768];
CACHE_ALIGN u32 color_555_to_888[32768];

//is this a crazy idea? this table spreads 5 bits evenly over 31 from exactly 0 to INT_MAX
CACHE_ALIGN const u32 material_5bit_to_31bit[] = {
	0x00000000, 0x04210842, 0x08421084, 0x0C6318C6,
	0x10842108, 0x14A5294A, 0x18C6318C, 0x1CE739CE,
	0x21084210, 0x25294A52, 0x294A5294, 0x2D6B5AD6,
	0x318C6318, 0x35AD6B5A, 0x39CE739C, 0x3DEF7BDE,
	0x42108421, 0x46318C63, 0x4A5294A5, 0x4E739CE7,
	0x5294A529, 0x56B5AD6B, 0x5AD6B5AD, 0x5EF7BDEF,
	0x6318C631, 0x6739CE73, 0x6B5AD6B5, 0x6F7BDEF7,
	0x739CE739, 0x77BDEF7B, 0x7BDEF7BD, 0x7FFFFFFF
};

// 5-bit to 6-bit conversions use this formula -- dst = (src == 0) ? 0 : (2*src) + 1
// Reference GBATEK: http://problemkaputt.de/gbatek.htm#ds3dtextureblending
CACHE_ALIGN const u8 material_5bit_to_6bit[] = {
	0x00, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
	0x11, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F,
	0x21, 0x23, 0x25, 0x27, 0x29, 0x2B, 0x2D, 0x2F,
	0x31, 0x33, 0x35, 0x37, 0x39, 0x3B, 0x3D, 0x3F
};

CACHE_ALIGN const u8 material_5bit_to_8bit[] = {
	0x00, 0x08, 0x10, 0x18, 0x21, 0x29, 0x31, 0x39,
	0x42, 0x4A, 0x52, 0x5A, 0x63, 0x6B, 0x73, 0x7B,
	0x84, 0x8C, 0x94, 0x9C, 0xA5, 0xAD, 0xB5, 0xBD,
	0xC6, 0xCE, 0xD6, 0xDE, 0xE7, 0xEF, 0xF7, 0xFF
};

CACHE_ALIGN const u8 material_6bit_to_8bit[] = {
	0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
	0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
	0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D,
	0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D,
	0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E,
	0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
	0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF,
	0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF
};

CACHE_ALIGN const u8 material_3bit_to_8bit[] = {
	0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF
};

//maybe not very precise
CACHE_ALIGN const u8 material_3bit_to_5bit[] = {
	0, 4, 8, 13, 17, 22, 26, 31
};

//TODO - generate this in the static init method more accurately
CACHE_ALIGN const u8 material_3bit_to_6bit[] = {
	0, 8, 16, 26, 34, 44, 52, 63
};

void ColorspaceHandlerInit()
{
	static bool needInitTables = true;
	
	if (needInitTables)
	{
#define RGB15TO18_BITLOGIC(col)         ( (material_5bit_to_6bit[((col)>>10)&0x1F]<<16) | (material_5bit_to_6bit[((col)>>5)&0x1F]<<8) |  material_5bit_to_6bit[(col)&0x1F] )
#define RGB15TO18_SWAP_RB_BITLOGIC(col) (  material_5bit_to_6bit[((col)>>10)&0x1F]      | (material_5bit_to_6bit[((col)>>5)&0x1F]<<8) | (material_5bit_to_6bit[(col)&0x1F]<<16) )
#define RGB15TO24_BITLOGIC(col)         ( (material_5bit_to_8bit[((col)>>10)&0x1F]<<16) | (material_5bit_to_8bit[((col)>>5)&0x1F]<<8) |  material_5bit_to_8bit[(col)&0x1F] )
#define RGB15TO24_SWAP_RB_BITLOGIC(col) (  material_5bit_to_8bit[((col)>>10)&0x1F]      | (material_5bit_to_8bit[((col)>>5)&0x1F]<<8) | (material_5bit_to_8bit[(col)&0x1F]<<16) )
		
		for (size_t i = 0; i < 32768; i++)
		{
			color_555_to_666[i]					= LE_TO_LOCAL_32( RGB15TO18_BITLOGIC(i) );
			color_555_to_6665_opaque[i]			= LE_TO_LOCAL_32( RGB15TO18_BITLOGIC(i) | 0x1F000000 );
			color_555_to_6665_opaque_swap_rb[i]	= LE_TO_LOCAL_32( RGB15TO18_SWAP_RB_BITLOGIC(i) | 0x1F000000 );
			
			color_555_to_888[i]					= LE_TO_LOCAL_32( RGB15TO24_BITLOGIC(i) );
			color_555_to_8888_opaque[i]			= LE_TO_LOCAL_32( RGB15TO24_BITLOGIC(i) | 0xFF000000 );
			color_555_to_8888_opaque_swap_rb[i]	= LE_TO_LOCAL_32( RGB15TO24_SWAP_RB_BITLOGIC(i) | 0xFF000000 );
		}
	}
}

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

template <bool SWAP_RB, bool IS_UNALIGNED>
void ColorspaceConvertBuffer555To8888Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount)
{
	size_t i = 0;
	
#ifdef USEMANUALVECTORIZATION
	
#if defined(USEVECTORSIZE_128)
	const size_t pixCountVector = pixCount - (pixCount % 8);
#elif defined(USEVECTORSIZE_256)
	const size_t pixCountVector = pixCount - (pixCount % 16);
#elif defined(USEVECTORSIZE_512)
	const size_t pixCountVector = pixCount - (pixCount % 32);
#endif
	
	if (SWAP_RB)
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer555To8888Opaque_SwapRB_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer555To8888Opaque_SwapRB(src, dst, pixCountVector);
		}
	}
	else
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer555To8888Opaque_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer555To8888Opaque(src, dst, pixCountVector);
		}
	}
	
#pragma LOOPVECTORIZE_DISABLE
	
#endif // USEMANUALVECTORIZATION
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert555To8888Opaque<SWAP_RB>(src[i]);
	}
}

template <bool SWAP_RB, bool IS_UNALIGNED>
void ColorspaceConvertBuffer555To6665Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount)
{
	size_t i = 0;
	
#ifdef USEMANUALVECTORIZATION
	
#if defined(USEVECTORSIZE_128)
	const size_t pixCountVector = pixCount - (pixCount % 8);
#elif defined(USEVECTORSIZE_256)
	const size_t pixCountVector = pixCount - (pixCount % 16);
#elif defined(USEVECTORSIZE_512)
	const size_t pixCountVector = pixCount - (pixCount % 32);
#endif
	
	if (SWAP_RB)
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer555To6665Opaque_SwapRB_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer555To6665Opaque_SwapRB(src, dst, pixCountVector);
		}
	}
	else
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer555To6665Opaque_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer555To6665Opaque(src, dst, pixCountVector);
		}
	}
	
#pragma LOOPVECTORIZE_DISABLE
	
#endif // USEMANUALVECTORIZATION
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert555To6665Opaque<SWAP_RB>(src[i]);
	}
}

template <bool SWAP_RB, bool IS_UNALIGNED>
void ColorspaceConvertBuffer8888To6665(const u32 *src, u32 *dst, size_t pixCount)
{
	size_t i = 0;
	
#ifdef USEMANUALVECTORIZATION
	
#if defined(USEVECTORSIZE_128)
	const size_t pixCountVector = pixCount - (pixCount % 4);
#elif defined(USEVECTORSIZE_256)
	const size_t pixCountVector = pixCount - (pixCount % 8);
#elif defined(USEVECTORSIZE_512)
	const size_t pixCountVector = pixCount - (pixCount % 16);
#endif
	
	if (SWAP_RB)
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer8888To6665_SwapRB_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer8888To6665_SwapRB(src, dst, pixCountVector);
		}
	}
	else
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer8888To6665_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer8888To6665(src, dst, pixCountVector);
		}
	}
	
#pragma LOOPVECTORIZE_DISABLE
	
#endif // USEMANUALVECTORIZATION
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert8888To6665<SWAP_RB>(src[i]);
	}
}

template <bool SWAP_RB, bool IS_UNALIGNED>
void ColorspaceConvertBuffer6665To8888(const u32 *src, u32 *dst, size_t pixCount)
{
	size_t i = 0;
	
#ifdef USEMANUALVECTORIZATION
	
#if defined(USEVECTORSIZE_128)
	const size_t pixCountVector = pixCount - (pixCount % 4);
#elif defined(USEVECTORSIZE_256)
	const size_t pixCountVector = pixCount - (pixCount % 8);
#elif defined(USEVECTORSIZE_512)
	const size_t pixCountVector = pixCount - (pixCount % 16);
#endif
	
	if (SWAP_RB)
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer6665To8888_SwapRB_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer6665To8888_SwapRB(src, dst, pixCountVector);
		}
	}
	else
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer6665To8888_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer6665To8888(src, dst, pixCountVector);
		}
	}
	
#pragma LOOPVECTORIZE_DISABLE
	
#endif // USEMANUALVECTORIZATION
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert6665To8888<SWAP_RB>(src[i]);
	}
}

template <bool SWAP_RB, bool IS_UNALIGNED>
void ColorspaceConvertBuffer8888To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount)
{
	size_t i = 0;
	
#ifdef USEMANUALVECTORIZATION
	
#if defined(USEVECTORSIZE_128)
	const size_t pixCountVector = pixCount - (pixCount % 8);
#elif defined(USEVECTORSIZE_256)
	const size_t pixCountVector = pixCount - (pixCount % 16);
#elif defined(USEVECTORSIZE_512)
	const size_t pixCountVector = pixCount - (pixCount % 32);
#endif
	
	if (SWAP_RB)
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer8888To5551_SwapRB_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer8888To5551_SwapRB(src, dst, pixCountVector);
		}
	}
	else
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer8888To5551_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer8888To5551(src, dst, pixCountVector);
		}
	}
	
#pragma LOOPVECTORIZE_DISABLE
	
#endif // USEMANUALVECTORIZATION
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert8888To5551<SWAP_RB>(src[i]);
	}
}

template <bool SWAP_RB, bool IS_UNALIGNED>
void ColorspaceConvertBuffer6665To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount)
{
	size_t i = 0;
	
#ifdef USEMANUALVECTORIZATION
	
#if defined(USEVECTORSIZE_128)
	const size_t pixCountVector = pixCount - (pixCount % 8);
#elif defined(USEVECTORSIZE_256)
	const size_t pixCountVector = pixCount - (pixCount % 16);
#elif defined(USEVECTORSIZE_512)
	const size_t pixCountVector = pixCount - (pixCount % 32);
#endif
		
	if (SWAP_RB)
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer6665To5551_SwapRB_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer6665To5551_SwapRB(src, dst, pixCountVector);
		}
	}
	else
	{
		if (IS_UNALIGNED)
		{
			i = csh.ConvertBuffer6665To5551_IsUnaligned(src, dst, pixCountVector);
		}
		else
		{
			i = csh.ConvertBuffer6665To5551(src, dst, pixCountVector);
		}
	}
	
#pragma LOOPVECTORIZE_DISABLE
	
#endif // USEMANUALVECTORIZATION
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert6665To5551<SWAP_RB>(src[i]);
	}
}

size_t ColorspaceHandler::ConvertBuffer555To8888Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert555To8888Opaque<false>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer555To8888Opaque_SwapRB(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert555To8888Opaque<true>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer555To8888Opaque_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer555To8888Opaque(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer555To8888Opaque_SwapRB_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer555To8888Opaque_SwapRB(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer555To6665Opaque(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert555To6665Opaque<false>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer555To6665Opaque_SwapRB(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert555To6665Opaque<true>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer555To6665Opaque_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer555To6665Opaque(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer555To6665Opaque_SwapRB_IsUnaligned(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer555To6665Opaque_SwapRB(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer8888To6665(const u32 *src, u32 *dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert8888To6665<false>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer8888To6665_SwapRB(const u32 *src, u32 *dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert8888To6665<true>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer8888To6665_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer8888To6665(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer8888To6665_SwapRB_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer8888To6665_SwapRB(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer6665To8888(const u32 *src, u32 *dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert6665To8888<false>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer6665To8888_SwapRB(const u32 *src, u32 *dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert6665To8888<true>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer6665To8888_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer6665To8888(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer6665To8888_SwapRB_IsUnaligned(const u32 *src, u32 *dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer6665To8888_SwapRB(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer8888To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert8888To5551<false>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer8888To5551_SwapRB(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert8888To5551<true>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer8888To5551_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer8888To5551(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer8888To5551_SwapRB_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer8888To5551_SwapRB(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer6665To5551(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (; i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert6665To5551<false>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer6665To5551_SwapRB(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	size_t i = 0;
	
	for (;i < pixCount; i++)
	{
		dst[i] = ColorspaceConvert6665To5551<true>(src[i]);
	}
	
	return i;
}

size_t ColorspaceHandler::ConvertBuffer6665To5551_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer6665To5551(src, dst, pixCount);
}

size_t ColorspaceHandler::ConvertBuffer6665To5551_SwapRB_IsUnaligned(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount) const
{
	return this->ColorspaceHandler::ConvertBuffer6665To5551_SwapRB(src, dst, pixCount);
}

template u32 ColorspaceConvert555To8888Opaque<true>(const u16 src);
template u32 ColorspaceConvert555To8888Opaque<false>(const u16 src);

template u32 ColorspaceConvert555To6665Opaque<true>(const u16 src);
template u32 ColorspaceConvert555To6665Opaque<false>(const u16 src);

template u32 ColorspaceConvert8888To6665<true>(FragmentColor srcColor);
template u32 ColorspaceConvert8888To6665<false>(FragmentColor srcColor);

template u32 ColorspaceConvert8888To6665<true>(u32 srcColor);
template u32 ColorspaceConvert8888To6665<false>(u32 srcColor);

template u32 ColorspaceConvert6665To8888<true>(FragmentColor srcColor);
template u32 ColorspaceConvert6665To8888<false>(FragmentColor srcColor);

template u32 ColorspaceConvert6665To8888<true>(u32 srcColor);
template u32 ColorspaceConvert6665To8888<false>(u32 srcColor);

template u16 ColorspaceConvert8888To5551<true>(FragmentColor srcColor);
template u16 ColorspaceConvert8888To5551<false>(FragmentColor srcColor);

template u16 ColorspaceConvert8888To5551<true>(u32 srcColor);
template u16 ColorspaceConvert8888To5551<false>(u32 srcColor);

template u16 ColorspaceConvert6665To5551<true>(FragmentColor srcColor);
template u16 ColorspaceConvert6665To5551<false>(FragmentColor srcColor);

template u16 ColorspaceConvert6665To5551<true>(u32 srcColor);
template u16 ColorspaceConvert6665To5551<false>(u32 srcColor);

template void ColorspaceConvertBuffer555To8888Opaque<true, true>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer555To8888Opaque<true, false>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer555To8888Opaque<false, true>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer555To8888Opaque<false, false>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);

template void ColorspaceConvertBuffer555To6665Opaque<true, true>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer555To6665Opaque<true, false>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer555To6665Opaque<false, true>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer555To6665Opaque<false, false>(const u16 *__restrict src, u32 *__restrict dst, size_t pixCount);

template void ColorspaceConvertBuffer8888To6665<true, true>(const u32 *src, u32 *dst, size_t pixCount);
template void ColorspaceConvertBuffer8888To6665<true, false>(const u32 *src, u32 *dst, size_t pixCount);
template void ColorspaceConvertBuffer8888To6665<false, true>(const u32 *src, u32 *dst, size_t pixCount);
template void ColorspaceConvertBuffer8888To6665<false, false>(const u32 *src, u32 *dst, size_t pixCount);

template void ColorspaceConvertBuffer6665To8888<true, true>(const u32 *src, u32 *dst, size_t pixCount);
template void ColorspaceConvertBuffer6665To8888<true, false>(const u32 *src, u32 *dst, size_t pixCount);
template void ColorspaceConvertBuffer6665To8888<false, true>(const u32 *src, u32 *dst, size_t pixCount);
template void ColorspaceConvertBuffer6665To8888<false, false>(const u32 *src, u32 *dst, size_t pixCount);

template void ColorspaceConvertBuffer8888To5551<true, true>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer8888To5551<true, false>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer8888To5551<false, true>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer8888To5551<false, false>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);

template void ColorspaceConvertBuffer6665To5551<true, true>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer6665To5551<true, false>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer6665To5551<false, true>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
template void ColorspaceConvertBuffer6665To5551<false, false>(const u32 *__restrict src, u16 *__restrict dst, size_t pixCount);
