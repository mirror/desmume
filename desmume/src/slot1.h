/*
	Copyright (C) 2010-2011 DeSmuME team

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

#ifndef __SLOT1_H__
#define __SLOT1_H__

#include <string>
#include "common.h"
#include "types.h"
#include "debug.h"

class EMUFILE;

struct SLOT1INTERFACE
{
	// The name of the plugin, this name will appear in the plugins list
	const char * name;

	//called once when the plugin starts up
	BOOL (*init)(void);
	
	//called when the emulator resets
	void (*reset)(void);
	
	//called when the plugin shuts down
	void (*close)(void);
	
	//called when the user configurating plugin
	void (*config)(void);

	//called when the emulator write to addon
	void (*write08)(u8 PROCNUM, u32 adr, u8 val);
	void (*write16)(u8 PROCNUM, u32 adr, u16 val);
	void (*write32)(u8 PROCNUM, u32 adr, u32 val);

	//called when the emulator read from addon
	u8  (*read08)(u8 PROCNUM, u32 adr);
	u16 (*read16)(u8 PROCNUM, u32 adr);
	u32 (*read32)(u8 PROCNUM, u32 adr);
	
	//called when the user get info about addon pak (description)
	void (*info)(char *info);
}; 

enum NDS_SLOT1_TYPE
{
	NDS_SLOT1_NONE,
	NDS_SLOT1_RETAIL,
	NDS_SLOT1_R4,
	NDS_SLOT1_RETAIL_NAND,		// used in Made in Ore/WarioWare D.I.Y.
	NDS_SLOT1_COUNT		// use for counter addons - MUST TO BE LAST!!!
};

extern SLOT1INTERFACE slot1_device;						// current slot1 device
extern SLOT1INTERFACE slot1List[NDS_SLOT1_COUNT];
extern u8 slot1_device_type;

BOOL slot1Init();
void slot1Close();
void slot1Reset();
BOOL slot1Change(NDS_SLOT1_TYPE type);				// change current adddon
void slot1SetFatDir(const std::string& dir);
std::string slot1GetFatDir();
EMUFILE* slot1GetFatImage();
#endif //__SLOT1_H__
