/*  
	Copyright (C) 2006-2007 shash
	Copyright (C) 2007-2012 DeSmuME team

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

#ifndef MATRIX_H
#define MATRIX_H

#include <math.h>
#include <string.h>

#include "types.h"
#include "mem.h"

#ifdef ENABLE_SSE
#include <xmmintrin.h>
#endif

#ifdef ENABLE_SSE2
#include <emmintrin.h>
#endif

struct MatrixStack
{
	MatrixStack(int size, int type);
	s32		*matrix;
	s32		position;
	s32		size;
	u8		type;
};

void	MatrixInit				(float *matrix);
void	MatrixInit				(s32 *matrix);

//In order to conditionally use these asm optimized functions in visual studio
//without having to make new build types to exclude the assembly files.
//a bit sloppy, but there aint much to it

float	MatrixGetMultipliedIndex	(const u32 index, float *matrix, float *rightMatrix);
s32	MatrixGetMultipliedIndex	(const u32 index, s32 *matrix, s32 *rightMatrix);
void	MatrixSet				(s32 *matrix, int x, int y, s32 value);
void	MatrixCopy				(s32 * matrixDST, const s32 * matrixSRC);
int		MatrixCompare				(const s32 * matrixDST, const float * matrixSRC);
void	MatrixIdentity			(s32 *matrix);

void	MatrixStackInit				(MatrixStack *stack);
void	MatrixStackSetMaxSize		(MatrixStack *stack, int size);
void	MatrixStackPushMatrix		(MatrixStack *stack, const s32 *ptr);
void	MatrixStackPopMatrix		(s32 *mtxCurr, MatrixStack *stack, int size);
s32*	MatrixStackGetPos			(MatrixStack *stack, const size_t pos);
s32*	MatrixStackGet				(MatrixStack *stack);
void	MatrixStackLoadMatrix		(MatrixStack *stack, const size_t pos, const s32 *ptr);

void Vector2Copy(float *dst, const float *src);
void Vector2Add(float *dst, const float *src);
void Vector2Subtract(float *dst, const float *src);
float Vector2Dot(const float *a, const float *b);
float Vector2Cross(const float *a, const float *b);

float Vector3Dot(const float *a, const float *b);
void Vector3Cross(float* dst, const float *a, const float *b);
float Vector3Length(const float *a);
void Vector3Add(float *dst, const float *src);
void Vector3Subtract(float *dst, const float *src);
void Vector3Scale(float *dst, const float scale);
void Vector3Copy(float *dst, const float *src);
void Vector3Normalize(float *dst);

void Vector4Copy(float *dst, const float *src);

//these functions are an unreliable, inaccurate floor.
//it should only be used for positive numbers
//this isnt as fast as it could be if we used a visual c++ intrinsic, but those appear not to be universally available
FORCEINLINE u32 u32floor(float f)
{
#ifdef ENABLE_SSE2
	return (u32)_mm_cvtt_ss2si(_mm_set_ss(f));
#else
	return (u32)f;
#endif
}
FORCEINLINE u32 u32floor(double d)
{
#ifdef ENABLE_SSE2
	return (u32)_mm_cvttsd_si32(_mm_set_sd(d));
#else
	return (u32)d;
#endif
}

//same as above but works for negative values too.
//be sure that the results are the same thing as floorf!
FORCEINLINE s32 s32floor(float f)
{
#ifdef ENABLE_SSE2
	return _mm_cvtss_si32( _mm_add_ss(_mm_set_ss(-0.5f),_mm_add_ss(_mm_set_ss(f), _mm_set_ss(f))) ) >> 1;
#else
	return (s32)floorf(f);
#endif
}
FORCEINLINE s32 s32floor(double d)
{
	return s32floor((float)d);
}

//switched SSE2 functions
//-------------
#ifdef ENABLE_SSE2

static void memset_u16(void *dst, const u16 val, const size_t length)
{
	__m128i *dst_vec128 = (__m128i *)dst;
	const __m128i val_vec128 = _mm_set1_epi16(val);
	const size_t length_vec128 = length / (sizeof(val_vec128) / sizeof(val));
	
	for (size_t i = 0; i < length_vec128; i++)
		_mm_stream_si128(dst_vec128 + i, val_vec128);
}

template <size_t LENGTH>
static void memset_u16_fast(void *dst, const u16 val)
{
	__m128i *dst_vec128 = (__m128i *)dst;
	const __m128i val_vec128 = _mm_set1_epi16(val);
	MACRODO_N(LENGTH / (sizeof(val_vec128) / sizeof(val)), _mm_store_si128(dst_vec128 + (X), val_vec128));
}

static void memset_u32(void *dst, const u32 val, const size_t length)
{
	__m128i *dst_vec128 = (__m128i *)dst;
	const __m128i val_vec128 = _mm_set1_epi32(val);
	const size_t length_vec128 = length / (sizeof(val_vec128) / sizeof(val));
	
	for (size_t i = 0; i < length_vec128; i++)
		_mm_stream_si128(dst_vec128 + i, val_vec128);
}

template <size_t LENGTH>
static void memset_u32_fast(void *dst, const u32 val)
{
	__m128i *dst_vec128 = (__m128i *)dst;
	const __m128i val_vec128 = _mm_set1_epi32(val);
	MACRODO_N(LENGTH / (sizeof(val_vec128) / sizeof(val)), _mm_store_si128(dst_vec128 + (X), val_vec128));
}

#else //no sse2

static void memset_u16(void *dst, const u16 val, const size_t length)
{
#ifdef HOST_64
	u64 *dst_u64 = (u64 *)dst;
	const u64 val_u64 = ((u64)val << 48) | ((u64)val << 32) | ((u64)val << 16) | (u64)val;
	const size_t length_u64 = length / (sizeof(val_u64) / sizeof(val));
	
	for (size_t i = 0; i < length_u64; i++)
		dst_u64[i] = val_u64;
#else
	for (size_t i = 0; i < length; i++)
		((u16 *)dst)[i] = val;
#endif
}

template <size_t LENGTH>
static void memset_u16_fast(void *dst, const u16 val)
{
#ifdef HOST_64
	u64 *dst_u64 = (u64 *)dst;
	const u64 val_u64 = ((u64)val << 48) | ((u64)val << 32) | ((u64)val << 16) | (u64)val;
	MACRODO_N(LENGTH / (sizeof(val_u64) / sizeof(val)), (dst_u64[(X)] = val_u64));
#else
	for (size_t i = 0; i < LENGTH; i++)
		((u16 *)dst)[i] = val;
#endif
}

static void memset_u32(void *dst, const u32 val, const size_t length)
{
#ifdef HOST_64
	u64 *dst_u64 = (u64 *)dst;
	const u64 val_u64 = ((u64)val << 32) | (u64)val;
	const size_t length_u64 = length / (sizeof(val_u64) / sizeof(val));
	
	for (size_t i = 0; i < length_u64; i++)
		dst_u64[i] = val_u64;
#else
	for (size_t i = 0; i < length; i++)
		((u32 *)dst)[i] = val;
#endif
}

template <size_t LENGTH>
static void memset_u32_fast(void *dst, const u32 val)
{
#ifdef HOST_64
	u64 *dst_u64 = (u64 *)dst;
	const u64 val_u64 = ((u64)val << 32) | (u64)val;
	MACRODO_N(LENGTH / (sizeof(val_u64) / sizeof(val)), (dst_u64[(X)] = val_u64));
#else
	for (size_t i = 0; i < LENGTH; i++)
		((u16 *)dst)[i] = val;
#endif
}

#endif // ENABLE_SSE2

// NOSSE version always used in gfx3d.cpp
void _NOSSE_MatrixMultVec4x4 (const float *matrix, float *vecPtr);
void MatrixMultVec3x3_fixed(const s32 *matrix, s32 *vecPtr);

//---------------------------
//switched SSE functions
#ifdef ENABLE_SSE

struct SSE_MATRIX
{
	SSE_MATRIX(const float *matrix)
		: row0(_mm_load_ps(matrix))
		, row1(_mm_load_ps(matrix+4))
		, row2(_mm_load_ps(matrix+8))
		, row3(_mm_load_ps(matrix+12))
	{}

	union {
		__m128 rows[4];
		struct { __m128 row0; __m128 row1; __m128 row2; __m128 row3; };
	};
		
};

FORCEINLINE __m128 _util_MatrixMultVec4x4_(const SSE_MATRIX &mat, __m128 vec)
{
	__m128 xmm5 = _mm_shuffle_ps(vec, vec, B8(01010101));
	__m128 xmm6 = _mm_shuffle_ps(vec, vec, B8(10101010));
	__m128 xmm7 = _mm_shuffle_ps(vec, vec, B8(11111111));
	__m128 xmm4 = _mm_shuffle_ps(vec, vec, B8(00000000));

	xmm4 = _mm_mul_ps(xmm4,mat.row0);
	xmm5 = _mm_mul_ps(xmm5,mat.row1);
	xmm6 = _mm_mul_ps(xmm6,mat.row2);
	xmm7 = _mm_mul_ps(xmm7,mat.row3);
	xmm4 = _mm_add_ps(xmm4,xmm5);
	xmm4 = _mm_add_ps(xmm4,xmm6);
	xmm4 = _mm_add_ps(xmm4,xmm7);
	return xmm4;
}

FORCEINLINE void MatrixMultiply(float * matrix, const float * rightMatrix)
{
	//this seems to generate larger code, including many movaps, but maybe it is less harsh on the registers than the
	//more hand-tailored approach
	__m128 row0 = _util_MatrixMultVec4x4_((SSE_MATRIX)matrix,_mm_load_ps(rightMatrix)); 
	__m128 row1 = _util_MatrixMultVec4x4_((SSE_MATRIX)matrix,_mm_load_ps(rightMatrix+4));
	__m128 row2 = _util_MatrixMultVec4x4_((SSE_MATRIX)matrix,_mm_load_ps(rightMatrix+8)); 
	__m128 row3 = _util_MatrixMultVec4x4_((SSE_MATRIX)matrix,_mm_load_ps(rightMatrix+12));
	_mm_store_ps(matrix,row0); 
	_mm_store_ps(matrix+4,row1); 
	_mm_store_ps(matrix+8,row2);
	_mm_store_ps(matrix+12,row3);
}

FORCEINLINE void MatrixMultVec4x4(const float *matrix, float *vecPtr)
{
	_mm_store_ps(vecPtr,_util_MatrixMultVec4x4_((SSE_MATRIX)matrix,_mm_load_ps(vecPtr)));
}

FORCEINLINE void MatrixMultVec4x4_M2(const float *matrix, float *vecPtr)
{
	//there are hardly any gains from merging these manually
	MatrixMultVec4x4(matrix+16,vecPtr);
	MatrixMultVec4x4(matrix,vecPtr);
}

FORCEINLINE void MatrixMultVec3x3(const float * matrix, float * vecPtr)
{
	const __m128 vec = _mm_load_ps(vecPtr);

	__m128 xmm5 = _mm_shuffle_ps(vec, vec, B8(01010101));
	__m128 xmm6 = _mm_shuffle_ps(vec, vec, B8(10101010));
	__m128 xmm4 = _mm_shuffle_ps(vec, vec, B8(00000000));

	const SSE_MATRIX mat(matrix);

	xmm4 = _mm_mul_ps(xmm4,mat.row0);
	xmm5 = _mm_mul_ps(xmm5,mat.row1);
	xmm6 = _mm_mul_ps(xmm6,mat.row2);
	xmm4 = _mm_add_ps(xmm4,xmm5);
	xmm4 = _mm_add_ps(xmm4,xmm6);

	_mm_store_ps(vecPtr,xmm4);
}

FORCEINLINE void MatrixTranslate(float *matrix, const float *ptr)
{
	__m128 xmm4 = _mm_load_ps(ptr);
	__m128 xmm5 = _mm_shuffle_ps(xmm4, xmm4, B8(01010101));
	__m128 xmm6 = _mm_shuffle_ps(xmm4, xmm4, B8(10101010));
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, B8(00000000));
	
	xmm4 = _mm_mul_ps(xmm4,_mm_load_ps(matrix));
	xmm5 = _mm_mul_ps(xmm5,_mm_load_ps(matrix+4));
	xmm6 = _mm_mul_ps(xmm6,_mm_load_ps(matrix+8));
	xmm4 = _mm_add_ps(xmm4,xmm5);
	xmm4 = _mm_add_ps(xmm4,xmm6);
	xmm4 = _mm_add_ps(xmm4,_mm_load_ps(matrix+12));
	_mm_store_ps(matrix+12,xmm4);
}

FORCEINLINE void MatrixScale(float *matrix, const float *ptr)
{
	__m128 xmm4 = _mm_load_ps(ptr);
	__m128 xmm5 = _mm_shuffle_ps(xmm4, xmm4, B8(01010101));
	__m128 xmm6 = _mm_shuffle_ps(xmm4, xmm4, B8(10101010));
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, B8(00000000));
	
	xmm4 = _mm_mul_ps(xmm4,_mm_load_ps(matrix));
	xmm5 = _mm_mul_ps(xmm5,_mm_load_ps(matrix+4));
	xmm6 = _mm_mul_ps(xmm6,_mm_load_ps(matrix+8));
	_mm_store_ps(matrix,xmm4);
	_mm_store_ps(matrix+4,xmm5);
	_mm_store_ps(matrix+8,xmm6);
}

template<int NUM_ROWS>
FORCEINLINE void vector_fix2float(float* matrix, const float divisor)
{
	CTASSERT(NUM_ROWS==3 || NUM_ROWS==4);

	const __m128 val = _mm_set_ps1(divisor);

	_mm_store_ps(matrix,_mm_div_ps(_mm_load_ps(matrix),val));
	_mm_store_ps(matrix+4,_mm_div_ps(_mm_load_ps(matrix+4),val));
	_mm_store_ps(matrix+8,_mm_div_ps(_mm_load_ps(matrix+8),val));
	if(NUM_ROWS==4)
		_mm_store_ps(matrix+12,_mm_div_ps(_mm_load_ps(matrix+12),val));
}

#else //no sse

void MatrixMultVec4x4 (const float *matrix, float *vecPtr);
void MatrixMultVec3x3(const float * matrix, float * vecPtr);
void MatrixMultiply(float * matrix, const float * rightMatrix);
void MatrixTranslate(float *matrix, const float *ptr);
void MatrixScale(float * matrix, const float * ptr);

FORCEINLINE void MatrixMultVec4x4_M2(const float *matrix, float *vecPtr)
{
	//there are hardly any gains from merging these manually
	MatrixMultVec4x4(matrix+16,vecPtr);
	MatrixMultVec4x4(matrix,vecPtr);
}

template<int NUM_ROWS>
FORCEINLINE void vector_fix2float(float* matrix, const float divisor)
{
	for(int i=0;i<NUM_ROWS*4;i++)
		matrix[i] /= divisor;
}

#endif //switched SSE functions

void MatrixMultVec4x4 (const s32 *matrix, s32 *vecPtr);

void MatrixMultVec4x4_M2(const s32 *matrix, s32 *vecPtr);

void MatrixMultiply(s32* matrix, const s32* rightMatrix);
void MatrixScale(s32 *matrix, const s32 *ptr);
void MatrixTranslate(s32 *matrix, const s32 *ptr);

#endif // MATRIX_H
