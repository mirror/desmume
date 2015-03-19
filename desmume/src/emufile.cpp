/*
The MIT License

Copyright (C) 2009-2015 DeSmuME team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "emufile.h"

#include <vector>

bool EMUFILE::readAllBytes(std::vector<u8>* dstbuf, const std::string& fname)
{
	EMUFILE_FILE file(fname.c_str(),"rb");
	if(file.fail()) return false;
	int size = file.size();
	dstbuf->resize(size);
	file.fread(&dstbuf->at(0),size);
	return true;
}

size_t EMUFILE_MEMORY::_fread(const void *ptr, size_t bytes){
	u32 remain = len-pos;
	u32 todo = std::min<u32>(remain,(u32)bytes);
	if(len==0)
	{
		failbit = true;
		return 0;
	}
	if(todo<=4)
	{
		u8* src = buf()+pos;
		u8* dst = (u8*)ptr;
		for(size_t i=0;i<todo;i++)
			*dst++ = *src++;
	}
	else
	{
		memcpy((void*)ptr,buf()+pos,todo);
	}
	pos += todo;
	if(todo<bytes)
		failbit = true;
	return todo;
}

void EMUFILE_FILE::truncate(s32 length)
{
	::fflush(fp);
	#ifdef HOST_WINDOWS 
		_chsize(_fileno(fp),length);
	#else
		ftruncate(fileno(fp),length);
	#endif
	fclose(fp);
	fp = NULL;
	open(fname.c_str(),mode);
}

int EMUFILE_FILE::fseek(int offset, int origin)
{
	//if the position cache is enabled, and the seek offset matches the known current position, then early exit.
	if(mPositionCacheEnabled)
	{
		if(origin == SEEK_SET)
		{
			if(mFilePosition == offset)
			{
				return 0;
			}
		}
	}

	mCondition = eCondition_Clean;

	int ret = ::fseek(fp, offset, origin);
 
	if(mPositionCacheEnabled)
		mFilePosition = ::ftell(fp);
 
	return ret;
}


int EMUFILE_FILE::ftell()
{
	if(mPositionCacheEnabled)
		return (int)mFilePosition;
	return (u32)::ftell(fp);
}

void EMUFILE_FILE::DemandCondition(eCondition cond)
{
	//allows switching between reading and writing; an fseek is required, under the circumstances

	if(mCondition == eCondition_Clean)
		goto CONCLUDE;
	if(mCondition == eCondition_Unknown)
		goto RESET;
	if(mCondition != cond)
		goto RESET;

	return;

RESET:
	::fseek(fp,::ftell(fp),SEEK_SET);
CONCLUDE:
	mCondition = cond;
}

size_t EMUFILE_FILE::_fread(const void *ptr, size_t bytes)
{
	DemandCondition(eCondition_Read);
	size_t ret = ::fread((void*)ptr, 1, bytes, fp);
	mFilePosition += ret;
	if(ret < bytes)
		failbit = true;
	return ret;
}

void EMUFILE_FILE::EnablePositionCache()
{
	mPositionCacheEnabled = true; 
	mFilePosition = ::ftell(fp);
}

size_t EMUFILE_FILE::fwrite(const void *ptr, size_t bytes)
{
	DemandCondition(eCondition_Write);
	size_t ret = ::fwrite((void*)ptr, 1, bytes, fp);
	mFilePosition += ret;
	if(ret < bytes)
		failbit = true;
	
	return ret;
}


EMUFILE* EMUFILE_FILE::memwrap()
{
	EMUFILE_MEMORY* mem = new EMUFILE_MEMORY(size());
	if(size()==0) return mem;
	fread(mem->buf(),size());
	return mem;
}

EMUFILE* EMUFILE_MEMORY::memwrap()
{
	return this;
}

void EMUFILE::write64le(u64* val)
{
	write64le(*val);
}

void EMUFILE::write64le(u64 val)
{
	val = LOCAL_TO_LE_64(val);
	fwrite(&val,8);
}


size_t EMUFILE::read64le(u64 *Bufo)
{
	u64 buf;
	if(fread((char*)&buf,8) != 8)
		return 0;
	
	*Bufo = LE_TO_LOCAL_64(buf);
	
	return 1;
}

u64 EMUFILE::read64le()
{
	u64 temp;
	read64le(&temp);
	return temp;
}

void EMUFILE::write32le(u32* val)
{
	write32le(*val);
}

void EMUFILE::write32le(u32 val)
{
	val = LOCAL_TO_LE_32(val);
	fwrite(&val,4);
}

size_t EMUFILE::read32le(s32* Bufo) { return read32le((u32*)Bufo); }

size_t EMUFILE::read32le(u32* Bufo)
{
	u32 buf;
	if(fread(&buf,4)<4)
		return 0;
	
	*Bufo = LE_TO_LOCAL_32(buf);
	
	return 1;
}

u32 EMUFILE::read32le()
{
	u32 ret;
	read32le(&ret);
	return ret;
}

void EMUFILE::write16le(u16* val)
{
	write16le(*val);
}

void EMUFILE::write16le(u16 val)
{
	val = LOCAL_TO_LE_16(val);
	fwrite(&val,2);
}

size_t EMUFILE::read16le(s16* Bufo) { return read16le((u16*)Bufo); }

size_t EMUFILE::read16le(u16* Bufo)
{
	u32 buf;
	if(fread(&buf,2)<2)
		return 0;
	
	*Bufo = LE_TO_LOCAL_16(buf);
	
	return 1;
}

u16 EMUFILE::read16le()
{
	u16 ret;
	read16le(&ret);
	return ret;
}

void EMUFILE::write8le(u8* val)
{
	write8le(*val);
}


void EMUFILE::write8le(u8 val)
{
	fwrite(&val,1);
}

size_t EMUFILE::read8le(u8* val)
{
	return fread(val,1);
}

u8 EMUFILE::read8le()
{
	u8 temp;
	fread(&temp,1);
	return temp;
}

void EMUFILE::writedouble(double* val)
{
	write64le(double_to_u64(*val));
}
void EMUFILE::writedouble(double val)
{
	write64le(double_to_u64(val));
}

double EMUFILE::readdouble()
{
	double temp;
	readdouble(&temp);
	return temp;
}

size_t EMUFILE::readdouble(double* val)
{
	u64 temp;
	size_t ret = read64le(&temp);
	*val = u64_to_double(temp);
	return ret;
}

void EMUFILE::writeMemoryStream(EMUFILE_MEMORY* ms)
{
	s32 size = (s32)ms->size();
	write32le(size);
	if(size>0)
	{
		std::vector<u8>* vec = ms->get_vec();
		fwrite(&vec->at(0),size);
	}
}

void EMUFILE::readMemoryStream(EMUFILE_MEMORY* ms)
{
	s32 size = read32le();
	if(size != 0)
	{
		std::vector<u8> temp(size);
		fread(&temp[0],size);
		ms->fwrite(&temp[0],size);
	}
}
