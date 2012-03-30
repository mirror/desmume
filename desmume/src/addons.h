/*
	Copyright (C) 2009-2011 DeSmuME team

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

#ifndef __ADDONS_H__
#define __ADDONS_H__

#include "common.h"
#include "types.h"
#include "debug.h"

struct ADDONINTERFACE
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
	void (*write08)(u32 procnum, u32 adr, u8 val);
	void (*write16)(u32 procnum, u32 adr, u16 val);
	void (*write32)(u32 procnum, u32 adr, u32 val);

	//called when the emulator read from addon
	u8  (*read08)(u32 procnum, u32 adr);
	u16 (*read16)(u32 procnum, u32 adr);
	u32 (*read32)(u32 procnum, u32 adr);
	
	//called when the user get info about addon pak (description)
	void (*info)(char *info);
}; 

enum NDS_ADDON_TYPE
{
	NDS_ADDON_NONE,
	NDS_ADDON_CFLASH,		// compact flash
	NDS_ADDON_RUMBLEPAK,	// rumble pack
	NDS_ADDON_GBAGAME,		// gba game in slot
	NDS_ADDON_GUITARGRIP,	// Guitar Grip
	NDS_ADDON_EXPMEMORY,	// Memory Expansion 
	NDS_ADDON_PIANO,		// Piano
  NDS_ADDON_PADDLE,
	//NDS_ADDON_EXTERNALMIC,
	NDS_ADDON_COUNT		// use for counter addons - MUST TO BE LAST!!!
};

enum ADDON_CFLASH_MODE
{
	ADDON_CFLASH_MODE_Path, ADDON_CFLASH_MODE_File, ADDON_CFLASH_MODE_RomPath
};

extern ADDON_CFLASH_MODE CFlash_Mode;
extern std::string CFlash_Path;
inline bool CFlash_IsUsingPath() { return CFlash_Mode==ADDON_CFLASH_MODE_Path || CFlash_Mode==ADDON_CFLASH_MODE_RomPath; }

extern ADDONINTERFACE addon;						// current pak
extern ADDONINTERFACE addonList[NDS_ADDON_COUNT];	// lists pointer on paks
extern NDS_ADDON_TYPE addon_type;								// current type pak

extern char GBAgameName[MAX_PATH];					// file name for GBA game (rom)
extern void (*FeedbackON)(BOOL enable);				// feedback on/off

extern BOOL addonsInit();							// Init addons
extern void addonsClose();							// Shutdown addons
extern void addonsReset();							// Reset addon
extern BOOL addonsChangePak(NDS_ADDON_TYPE type);				// change current adddon

extern void guitarGrip_setKey(bool green, bool red, bool yellow, bool blue); // Guitar grip keys
extern void piano_setKey(bool c, bool cs, bool d, bool ds, bool e, bool f, bool fs, bool g, bool gs, bool a, bool as, bool b, bool hic); //piano keys

#endif //__ADDONS_H__
