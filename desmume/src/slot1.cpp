/*
	Copyright (C) 2010-2012 DeSmuME team

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

#include <string>

#include "types.h"
#include "slot1.h"

#include "NDSSystem.h"
#include "emufile.h"
#include "utils/vfat.h"

extern SLOT1INTERFACE slot1None;
extern SLOT1INTERFACE slot1Retail;
extern SLOT1INTERFACE slot1R4;
extern SLOT1INTERFACE slot1Retail_NAND;

static EMUFILE* fatImage = NULL;
static std::string fatDir;

SLOT1INTERFACE slot1List[NDS_SLOT1_COUNT] = {
		slot1None,
		slot1Retail,
		slot1R4,
		slot1Retail_NAND
};

SLOT1INTERFACE slot1_device = slot1Retail; //default for frontends that dont even configure this
NDS_SLOT1_TYPE slot1_device_type = NDS_SLOT1_RETAIL;

static void scanDir()
{
	if(fatDir == "") return;
	
	if (fatImage)
	{
		delete fatImage;
		fatImage = NULL;
	}

	VFAT vfat;
	if(vfat.build(fatDir.c_str(),16))
	{
		fatImage = vfat.detach();
	}
}

BOOL slot1Init()
{
	if (slot1_device_type == NDS_SLOT1_R4)
		scanDir();
	return slot1_device.init();
}

void slot1Close()
{
	slot1_device.close();
	
	//be careful to do this second, maybe the device will write something more
	if (fatImage)
	{
		delete fatImage;
		fatImage = NULL;
	}
}

void slot1Reset()
{
	slot1_device.reset();
}

BOOL slot1Change(NDS_SLOT1_TYPE changeToType)
{
	if(changeToType == slot1_device_type) return FALSE; //nothing to do
	if (changeToType > NDS_SLOT1_COUNT || changeToType < 0) return FALSE;
	slot1_device.close();
	slot1_device_type = changeToType;
	slot1_device = slot1List[slot1_device_type];
	if (changeToType == NDS_SLOT1_R4)
		scanDir();
	printf("Slot 1: %s\n", slot1_device.name);
	printf("sending eject signal to SLOT-1\n");
	NDS_TriggerCardEjectIRQ();
	return slot1_device.init();
}

void slot1SetFatDir(const std::string& dir)
{
	//printf("FAT path %s\n", dir.c_str());
	fatDir = dir;
}

std::string slot1GetFatDir()
{
	return fatDir;
}

EMUFILE* slot1GetFatImage()
{
	return fatImage;
}

NDS_SLOT1_TYPE slot1GetCurrentType()
{
	return slot1_device_type;
}