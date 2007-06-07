/*  Copyright 2007 Guillaume Duhamel

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

#include "ROMReader.h"

#include <stdio.h>
#ifdef HAVE_LIBZZIP
#include <zzip/zzip.h>
#endif
#ifdef WIN32
#define strcasecmp stricmp
#endif

ROMReader_struct * ROMReaderInit(char ** filename)
{
#ifdef HAVE_LIBZ
	if(!strcasecmp(".gz", *filename + (strlen(*filename) - 3)))
	{
		(*filename)[strlen(*filename) - 3] = '\0';
		return &GZIPROMReader;
	}
#endif
#ifdef HAVE_LIBZZIP
	if (!strcasecmp(".zip", *filename + (strlen(*filename) - 4)))
	{
		(*filename)[strlen(*filename) - 4] = '\0';
		return &ZIPROMReader;
	}
#endif
	return &STDROMReader;
}

void * STDROMReaderInit(const char * filename);
void STDROMReaderDeInit(void *);
u32 STDROMReaderSize(void *);
int STDROMReaderSeek(void *, int, int);
int STDROMReaderRead(void *, void *, u32);

ROMReader_struct STDROMReader =
{
	ROMREADER_STD,
	"Standard ROM Reader",
	STDROMReaderInit,
	STDROMReaderDeInit,
	STDROMReaderSize,
	STDROMReaderSeek,
	STDROMReaderRead
};

void * STDROMReaderInit(const char * filename)
{
	return (void *) fopen(filename, "rb");
}

void STDROMReaderDeInit(void * file)
{
	if (!file) return ;
	fclose(file);
}

u32 STDROMReaderSize(void * file)
{
	u32 size;

	if (!file) return 0 ;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}

int STDROMReaderSeek(void * file, int offset, int whence)
{
	if (!file) return 0 ;
	return fseek(file, offset, whence);
}

int STDROMReaderRead(void * file, void * buffer, u32 size)
{
	if (!file) return 0 ;
	return fread(buffer, 1, size, file);
}

#ifdef HAVE_LIBZ
void * GZIPROMReaderInit(const char * filename);
void GZIPROMReaderDeInit(void *);
u32 GZIPROMReaderSize(void *);
int GZIPROMReaderSeek(void *, int, int);
int GZIPROMReaderRead(void *, void *, u32);

ROMReader_struct GZIPROMReader =
{
	ROMREADER_GZIP,
	"Gzip ROM Reader",
	GZIPROMReaderInit,
	GZIPROMReaderDeInit,
	GZIPROMReaderSize,
	GZIPROMReaderSeek,
	GZIPROMReaderRead
};

void * GZIPROMReaderInit(const char * filename)
{
	return (void*)gzopen(filename, "rb");
}

void GZIPROMReaderDeInit(void * file)
{
	gzclose(file);
}

u32 GZIPROMReaderSize(void * file)
{
	char useless[1024];
	u32 size = 0;

	/* FIXME this function should first save the current
	 * position and restore it after size calculation */
	gzrewind(file);
	while (gzeof (file) == 0)
		size += gzread(file, useless, 1024);
	gzrewind(file);

	return size;
}

int GZIPROMReaderSeek(void * file, int offset, int whence)
{
	return gzseek(file, offset, whence);
}

int GZIPROMReaderRead(void * file, void * buffer, u32 size)
{
	return gzread(file, buffer, size);
}
#endif

#ifdef HAVE_LIBZZIP
void * ZIPROMReaderInit(const char * filename);
void ZIPROMReaderDeInit(void *);
u32 ZIPROMReaderSize(void *);
int ZIPROMReaderSeek(void *, int, int);
int ZIPROMReaderRead(void *, void *, u32);

ROMReader_struct ZIPROMReader =
{
	ROMREADER_ZIP,
	"Zip ROM Reader",
	ZIPROMReaderInit,
	ZIPROMReaderDeInit,
	ZIPROMReaderSize,
	ZIPROMReaderSeek,
	ZIPROMReaderRead
};

void * ZIPROMReaderInit(const char * filename)
{
	ZZIP_DIRENT * dir = zzip_opendir(filename);
	dir = zzip_readdir(dir);
	if (dir != NULL)
	{
		char tmp1[1024];
		char tmp2[1024];
		strncpy(tmp1, filename, strlen(filename) - 4);
		sprintf(tmp2, "%s/%s", tmp1, dir->d_name);
		return zzip_fopen(tmp2, "rb");
	}
	return NULL;
}

void ZIPROMReaderDeInit(void * file)
{
	zzip_close(file);
}

u32 ZIPROMReaderSize(void * file)
{
	char useless[1024];
	u32 tmp;
	u32 size = 0;

	zzip_seek(file, 0, SEEK_END);
	size = zzip_tell(file);
	zzip_seek(file, 0, SEEK_SET);

	return size;
}

int ZIPROMReaderSeek(void * file, int offset, int whence)
{
	return zzip_seek(file, offset, whence);
}

int ZIPROMReaderRead(void * file, void * buffer, u32 size)
{
	return zzip_read(file, buffer, size);
}
#endif
