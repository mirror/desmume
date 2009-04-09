/*  Copyright (C) 2005 Guillaume Duhamel

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef TYPES_HPP
#define TYPES_HPP

//--------------
//configuration
#define DEVELOPER
//--------------

#define DESMUME_NAME "DeSmuME"

#ifdef _WIN64
#define DESMUME_PLATFORM_STRING " x64"
#else
#ifdef _WIN32
#define DESMUME_PLATFORM_STRING " x86"
#else
#define DESMUME_PLATFORM_STRING ""
#endif
#endif

#ifdef SSE2
#define DESMUME_CPUEXT_STRING " SSE2"
#else
#define DESMUME_CPUEXT_STRING ""
#endif

//#ifdef DEVELOPER
//#define DESMUME_FEATURE_STRING " dev+"
//#else
#define DESMUME_FEATURE_STRING ""
//#endif

#ifdef DEBUG
#define DESMUME_SUBVERSION_STRING " debug"
#else
#ifdef RELEASE
#define DESMUME_SUBVERSION_STRING ""
#else
#define DESMUME_SUBVERSION_STRING " prerelease"
#endif
#endif

#ifdef __INTEL_COMPILER
#define DESMUME_COMPILER " (Intel) "
#define DESMUME_COMPILER_DETAIL " (Intel) "
#elif defined(_MSC_VER)
#define DESMUME_COMPILER ""
#define DESMUME_COMPILER_DETAIL " msvc " _Py_STRINGIZE(_MSC_VER)
#define _Py_STRINGIZE(X) _Py_STRINGIZE1((X))
#define _Py_STRINGIZE1(X) _Py_STRINGIZE2 ## X
#define _Py_STRINGIZE2(X) #X
//re: http://72.14.203.104/search?q=cache:HG-okth5NGkJ:mail.python.org/pipermail/python-checkins/2002-November/030704.html+_msc_ver+compiler+version+string&hl=en&gl=us&ct=clnk&cd=5
#else
// TODO: make for others compilers
#define DESMUME_COMPILER ""
#define DESMUME_COMPILER_DETAIL ""
#endif

#define DESMUME_VERSION_NUMERIC 90200
#define DESMUME_VERSION_STRING " " "0.9.2" DESMUME_FEATURE_STRING DESMUME_PLATFORM_STRING DESMUME_CPUEXT_STRING DESMUME_SUBVERSION_STRING DESMUME_COMPILER
#define DESMUME_NAME_AND_VERSION " " DESMUME_NAME DESMUME_VERSION_STRING

#ifdef _WIN32
#define strcasecmp(x,y) _stricmp(x,y)
#else
#define WINAPI
#endif

#ifdef __GNUC__
#include <limits.h>
#define MAX_PATH PATH_MAX
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define ALIGN(X) __declspec(align(X))
#elif __GNUC__
#define ALIGN(X) __attribute__ ((aligned (X)))
#else
#define ALIGN(X)
#endif

#define CACHE_ALIGN ALIGN(32)

#ifndef FASTCALL
#ifdef __MINGW32__
#define FASTCALL __attribute__((fastcall))
#elif defined (__i386__)
#define FASTCALL __attribute__((regparm(3)))
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define FASTCALL
#else
#define FASTCALL
#endif
#endif

#ifndef INLINE
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define INLINE _inline
#else
#define INLINE inline
#endif
#endif

#ifndef FORCEINLINE
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE INLINE
#endif
#endif

#if defined(__LP64__)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned long pointer;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long s64;
#else
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif
typedef unsigned long pointer;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
typedef __int64 s64;
#else
typedef signed long long s64;
#endif
#endif

typedef u8  uint8;
typedef u16 uint16;

#ifndef OBJ_C
typedef u32 uint32;
#else
#define uint32 u32 //uint32 is defined in Leopard somewhere, avoid conflicts
#endif

/*---------- GPU3D fixed-points types -----------*/

typedef s32 f32;
#define inttof32(n)          ((n) << 12)
#define f32toint(n)          ((n) >> 12)
#define floattof32(n)        ((int32)((n) * (1 << 12)))
#define f32tofloat(n)        (((float)(n)) / (float)(1<<12))

typedef s16 t16;
#define f32tot16(n)          ((t16)(n >> 8))
#define inttot16(n)          ((n) << 4)
#define t16toint(n)          ((n) >> 4)
#define floattot16(n)        ((t16)((n) * (1 << 4)))
#define t16ofloat(n)         (((float)(n)) / (float)(1<<4))

typedef s16 v16;
#define inttov16(n)          ((n) << 12)
#define f32tov16(n)          (n)
#define floattov16(n)        ((v16)((n) * (1 << 12)))
#define v16toint(n)          ((n) >> 12)
#define v16tofloat(n)        (((float)(n)) / (float)(1<<12))

typedef s16 v10;
#define inttov10(n)          ((n) << 9)
#define f32tov10(n)          ((v10)(n >> 3))
#define v10toint(n)          ((n) >> 9)
#define floattov10(n)        ((v10)((n) * (1 << 9)))
#define v10tofloat(n)        (((float)(n)) / (float)(1<<9))

/*----------------------*/

#ifndef OBJ_C
typedef int BOOL;
#else
//apple also defines BOOL
typedef int desmume_BOOL;
#define BOOL desmume_BOOL
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef __BIG_ENDIAN__
#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN
#endif
#endif

#ifdef WORDS_BIGENDIAN
# define LOCAL_BE
#else
# define LOCAL_LE
#endif

/* little endian (ds' endianess) to local endianess convert macros */
#ifdef LOCAL_BE	/* local arch is big endian */
# define LE_TO_LOCAL_16(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
# define LE_TO_LOCAL_32(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
# define LE_TO_LOCAL_64(x) ((((x)&0xff)<<56)|(((x)&0xff00)<<40)|(((x)&0xff0000)<<24)|(((x)&0xff000000)<<8)|(((x)>>8)&0xff000000)|(((x)>>24)&0xff00)|(((x)>>40)&0xff00)|(((x)>>56)&0xff))
# define LOCAL_TO_LE_16(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
# define LOCAL_TO_LE_32(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
# define LOCAL_TO_LE_64(x) ((((x)&0xff)<<56)|(((x)&0xff00)<<40)|(((x)&0xff0000)<<24)|(((x)&0xff000000)<<8)|(((x)>>8)&0xff000000)|(((x)>>24)&0xff00)|(((x)>>40)&0xff00)|(((x)>>56)&0xff))
#else		/* local arch is little endian */
# define LE_TO_LOCAL_16(x) (x)
# define LE_TO_LOCAL_32(x) (x)
# define LE_TO_LOCAL_64(x) (x)
# define LOCAL_TO_LE_16(x) (x)
# define LOCAL_TO_LE_32(x) (x)
# define LOCAL_TO_LE_64(x) (x)
#endif

// kilobytes and megabytes macro
#define MB(x) ((x)*1024*1024)
#define KB(x) ((x)*1024)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define CPU_STR(c) ((c==ARM9)?"ARM9":"ARM7")
typedef enum
{
	ARM9 = 0,
	ARM7 = 1
} cpu_id_t;

///endian-flips count bytes.  count should be even and nonzero.
inline void FlipByteOrder(u8 *src, u32 count)
{
	u8 *start=src;
	u8 *end=src+count-1;

	if((count&1) || !count)        return;         /* This shouldn't happen. */

	while(count--)
	{
		u8 tmp;

		tmp=*end;
		*end=*start;
		*start=tmp;
		end--;
		start++;
	}
}



inline u64 double_to_u64(double d) {
	union {
		u64 a;
		double b;
	} fuxor;
	fuxor.b = d;
	return fuxor.a;
}

inline double u64_to_double(u64 u) {
	union {
		u64 a;
		double b;
	} fuxor;
	fuxor.a = u;
	return fuxor.b;
}


///stores a 32bit value into the provided byte array in guaranteed little endian form
inline void en32lsb(u8 *buf, u32 morp)
{ 
	buf[0]=morp;
	buf[1]=morp>>8;
	buf[2]=morp>>16;
	buf[3]=morp>>24;
} 

inline void en16lsb(u8* buf, u16 morp)
{
	buf[0]=morp;
	buf[1]=morp>>8;
}

///unpacks a 64bit little endian value from the provided byte array into host byte order
inline u64 de64lsb(u8 *morp)
{
	return morp[0]|(morp[1]<<8)|(morp[2]<<16)|(morp[3]<<24)|((u64)morp[4]<<32)|((u64)morp[5]<<40)|((u64)morp[6]<<48)|((u64)morp[7]<<56);
}

///unpacks a 32bit little endian value from the provided byte array into host byte order
inline u32 de32lsb(u8 *morp)
{
	return morp[0]|(morp[1]<<8)|(morp[2]<<16)|(morp[3]<<24);
}

///unpacks a 16bit little endian value from the provided byte array into host byte order
inline u16 de16lsb(u8 *morp)
{
	return morp[0]|(morp[1]<<8);
}

#ifndef ARRAY_SIZE
//taken from winnt.h
extern "C++" // templates cannot be declared to have 'C' linkage
template <typename T, size_t N>
char (*BLAHBLAHBLAH( UNALIGNED T (&)[N] ))[N];

#define ARRAY_SIZE(A) (sizeof(*BLAHBLAHBLAH(A)))
#endif



#endif
