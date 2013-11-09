/*
	Copyright (C) 2006 thoduv
	Copyright (C) 2006 Theo Berkau
	Copyright (C) 2008-2013 DeSmuME team

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

#ifndef __MC_H__
#define __MC_H__

#include <stdio.h>
#include <vector>
#include <string>
#include "types.h"
#include "emufile.h"
#include "common.h"
#include "utils/tinyxml/tinyxml.h"

#define MAX_SAVE_TYPES 13
#define MC_TYPE_AUTODETECT      0x0
#define MC_TYPE_EEPROM1         0x1
#define MC_TYPE_EEPROM2         0x2
#define MC_TYPE_FLASH           0x3
#define MC_TYPE_FRAM            0x4

#define MC_SIZE_4KBITS                  0x000200
#define MC_SIZE_64KBITS                 0x002000
#define MC_SIZE_256KBITS                0x008000
#define MC_SIZE_512KBITS                0x010000
#define MC_SIZE_1MBITS                  0x020000
#define MC_SIZE_2MBITS                  0x040000
#define MC_SIZE_4MBITS                  0x080000
#define MC_SIZE_8MBITS                  0x100000
#define MC_SIZE_16MBITS                 0x200000
#define MC_SIZE_32MBITS                 0x400000
#define MC_SIZE_64MBITS                 0x800000
#define MC_SIZE_128MBITS                0x1000000
#define MC_SIZE_256MBITS                0x2000000
#define MC_SIZE_512MBITS                0x4000000


//This "backup device" represents a typical retail NDS save memory accessible via AUXSPI.
//It is managed as a core emulator service for historical reasons which are bad,
//and possible infrastructural simplification reasons which are good.
//Slot-1 devices will map their AUXSPI accesses through to the core-managed BackupDevice to access it for the running software.
class BackupDevice
{
public:
	BackupDevice();

	//signals the save system that we are in MOVIE mode. doesnt load up a rom, and never saves it. initializes for that case.
	void movie_mode();

	void reset();
	void close_rom();
	void forceManualBackupType();
	void reset_hardware();
	std::string getFilename() { return filename; }
	u8 searchFileSaveType(u32 size);

	bool save_state(EMUFILE* os);
	bool load_state(EMUFILE* is);
	
	//commands from mmu
	void reset_command() { reset_command_state = true; };
	u8 data_command(u8, u8);
	std::vector<u8> data;

	//this info was saved before the last reset (used for savestate compatibility)
	struct SavedInfo
	{
		u32 addr_size;
	} savedInfo;

	//and these are used by old savestates
	void load_old_state(u32 addr_size, u8* data, u32 datasize);
	static u32 addr_size_for_old_save_size(int bupmem_size);
	static u32 addr_size_for_old_save_type(int bupmem_type);

	static u32 pad_up_size(u32 startSize);
	void raw_applyUserSettings(u32& size, bool manual = false);

	bool load_duc(const char* filename, u32 force_size = 0);
	bool load_no_gba(const char *fname, u32 force_size = 0);
	bool save_no_gba(const char* fname);
	bool load_raw(const char* filename, u32 force_size = 0);
	bool save_raw(const char* filename);
	bool load_movie(EMUFILE* is);
	u32 get_save_duc_size(const char* filename);
	u32 get_save_nogba_size(const char* filename);
	u32 get_save_raw_size(const char* filename);

	//call me once a second or so to lazy flush the save data
	//here's the reason for this system: we want to dump save files when theyre READ
	//so that we have a better idea earlier on how large they are. but it slows things down
	//way too much if we flush whenever we read.
	void lazy_flush();
	void flush();

	void setFlushPending() { flushPending = true; }
	void setLazyFlushPending() { lazyFlushPending = true; }

	void ensure(u32 addr);
	void ensure(u32 addr, u8 val);

	struct {
			u32 size,padSize,type,addr_size,mem_size;
		} info;

	bool isMovieMode;

	u32 importDataSize(const char *filename);
	u32 importData(const char *filename, u32 force_size = 0);
	bool exportData(const char *filename);
private:
	void detect();

	std::string filename;
	
	bool write_enable;	//is write enabled?
	u8 write_protect;
	bool reset_command_state;
	u32 com;	//persistent command actually handled
	u32 addr_size, addr_counter;
	u32 addr;

	std::vector<u8> data_autodetect;
	enum STATE {
		DETECTING = 0, RUNNING = 1
	} state;

	enum MOTION_INIT_STATE
	{
		MOTION_INIT_STATE_IDLE, MOTION_INIT_STATE_RECEIVED_4, MOTION_INIT_STATE_RECEIVED_4_B,
		MOTION_INIT_STATE_FE, MOTION_INIT_STATE_FD, MOTION_INIT_STATE_FB
	};
	enum MOTION_FLAG
	{
		MOTION_FLAG_NONE=0,
		MOTION_FLAG_ENABLED=1,
		MOTION_FLAG_SENSORMODE=2
	};
	u8 motionInitState, motionFlag;

	void loadfile();
	bool _loadfile(const char *fname);

	bool flushPending, lazyFlushPending;

	void resize(u32 size);
	void resize(u32 size, u8 val);

	void checkReset();
};




void backup_setManualBackupType(int type);
void backup_forceManualBackupType();

struct SAVE_TYPE
{
	const char* descr;
	int media_type;
	int size;
};

extern const SAVE_TYPE save_types[];

#endif /*__FW_H__*/

