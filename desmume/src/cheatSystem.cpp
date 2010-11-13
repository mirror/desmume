/*  Copyright 2009-2010 DeSmuME team

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <string.h>
#include "cheatSystem.h"
#include "NDSSystem.h"
#include "mem.h"
#include "MMU.h"
#include "debug.h"
#include "utils/xstring.h"

#ifndef _MSC_VER
#include <stdint.h>
#endif

CHEATS *cheats = NULL;
CHEATSEARCH *cheatSearch = NULL;

void CHEATS::clear()
{
	list.resize(0);
	currentGet = 0;
}

void CHEATS::init(char *path)
{
	clear();
	strcpy((char *)filename, path);

	load();
}

BOOL CHEATS::add(u8 size, u32 address, u32 val, char *description, BOOL enabled)
{
	size_t num = list.size();
	list.push_back(CHEATS_LIST());
	list[num].code[0][0] = address & 0x00FFFFFF;
	list[num].code[0][1] = val;
	list[num].num = 1;
	list[num].type = 0;
	list[num].size = size;
	strcpy(list[num].description, description);
	list[num].enabled = enabled;
	return TRUE;
}

BOOL CHEATS::update(u8 size, u32 address, u32 val, char *description, BOOL enabled, u32 pos)
{
	if (pos >= list.size()) return FALSE;
	list[pos].code[0][0] = address & 0x00FFFFFF;
	list[pos].code[0][1] = val;
	list[pos].num = 1;
	list[pos].type = 0;
	list[pos].size = size;
	strcpy(list[pos].description, description);
	list[pos].enabled = enabled;
	return TRUE;
}

void CHEATS::ARparser(CHEATS_LIST& list)
{
	u8	type = 0;
	u8	subtype = 0;
	u32	hi = 0;
	u32	lo = 0;
	u32	addr = 0;
	u32	val = 0;
	// AR temporary vars & flags
	u32	offset = 0;
	u32	datareg = 0;
	u32	loopcount = 0;
	u32	counter = 0;
	u32	if_flag = 0;
	s32 loopbackline = 0;
	u32 loop_flag = 0;
	
	for (int i=0; i < list.num; i++)
	{
		type = list.code[i][0] >> 28;
		subtype = (list.code[i][0] >> 24) & 0x0F;

		hi = list.code[i][0] & 0x0FFFFFFF;
		lo = list.code[i][1];

		if (if_flag > 0) 
		{
			if ( (type == 0x0D) && (subtype == 0)) if_flag--;	// ENDIF
			if ( (type == 0x0D) && (subtype == 2))				// NEXT & Flush
			{
				if (loop_flag)
					i = (loopbackline-1);
				else
				{
					offset = 0;
					datareg = 0;
					loopcount = 0;
					counter = 0;
					if_flag = 0;
					loop_flag = 0;
				}
			}
			continue;
		}

		switch (type)
		{
			case 0x00:
			{
				if (hi==0)
				{
					//manual hook
				}
				else
				if ((hi==0x0000AA99) && (lo==0))
				{
					//parameter bytes 9..10 for above code (padded with 00s)
				}
				else
				{
					addr = hi + offset;
					T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], lo);
				}
			}
			break;

			case 0x01:
				addr = hi + offset;
				T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], lo & 0x0000FFFF);
			break;

			case 0x02:
				addr = hi + offset;
				T1WriteByte(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], lo & 0x000000FF);
			break;

			case 0x03:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]);
				if ( lo > val )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x04:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]);
				if ( lo < val )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x05:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]);
				if ( lo == val )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x06:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]);
				if ( lo != val )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x07:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]) & 0x0000FFFF;
				if ( (lo & 0xFFFF) > ( (~(lo >> 16)) & val) )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x08:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]) & 0x0000FFFF;
				if ( (lo & 0xFFFF) < ( (~(lo >> 16)) & val) )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x09:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]);
				if ( (lo & 0xFFFF) == ( (~(lo >> 16)) & val) )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x0A:
				if (hi == 0) hi = offset;	// V1.54+
				val = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][hi>>20], hi & MMU.MMU_MASK[ARMCPU_ARM9][hi>>20]) & 0x0000FFFF;
				if ( (lo & 0xFFFF) != ( (~(lo >> 16)) & val) )
				{
					if (if_flag > 0) if_flag--;
				}
				else
				{
					if_flag++;
				}
			break;

			case 0x0B:
				addr = hi + offset;
				offset = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20]);
			break;

			case 0x0C:
				switch (subtype)
				{
					case 0x0:
						if (loopcount < (lo+1))
							loop_flag = 1;
						else
							loop_flag = 0;
						loopcount++;
						loopbackline = i;
					break;

					case 0x4:
					break;

					case 0x5:
						counter++;
						if ( (counter & (lo & 0xFFFF)) == ((lo >> 8) & 0xFFFF) )
						{
							if (if_flag > 0) if_flag--;
						}
						else
						{
							if_flag++;
						}
					break;

					case 0x6:
						T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][lo>>20], lo & MMU.MMU_MASK[ARMCPU_ARM9][lo>>20], offset);
					break;
				}
			break;

			case 0x0D:
			{
				switch (subtype)
				{
					case 0x0:
					break;

					case 0x1:
						if (loop_flag)
							i = (loopbackline-1);
					break;

					case 0x2:
						if (loop_flag)
							i = (loopbackline-1);
						else
						{
							offset = 0;
							datareg = 0;
							loopcount = 0;
							counter = 0;
							if_flag = 0;
							loop_flag = 0;
						}
					break;

					case 0x3:
						offset = lo;
					break;

					case 0x4:
						datareg += lo;
					break;

					case 0x5:
						datareg = lo;
					break;

					case 0x6:
						addr = lo + offset;
						T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], datareg);
						offset += 4;
					break;

					case 0x7:
						addr = lo + offset;
						T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], datareg & 0x0000FFFF);
						offset += 2;
					break;

					case 0x8:
						addr = lo + offset;
						T1WriteByte(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], datareg & 0x000000FF);
						offset += 1;
					break;

					case 0x9:
						addr = lo + offset;
						datareg = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20]);
					break;

					case 0xA:
						addr = lo + offset;
						datareg = T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20]) & 0x0000FFFF;
					break;

					case 0xB:
						addr = lo + offset;
						datareg = T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20]) & 0x000000FF;
					break;

					case 0xC:
						offset += lo;
					break;
				}
			}
			break;

			case 0xE:
			{
				u8	*tmp_code = (u8*)(list.code[i+1]);
				u32 addr = hi+offset;

				for (u32 t = 0; t < lo; t++)
				{
					u8	tmp = T1ReadByte(tmp_code, t);
					T1WriteByte(MMU.MMU_MEM[ARMCPU_ARM9][addr>>20], addr & MMU.MMU_MASK[ARMCPU_ARM9][addr>>20], tmp);
					addr++;
				}
				i += (lo / 8);
			}
			break;

			case 0xF:
				for (u32 t = 0; t < lo; t++)
				{
					u8 tmp = T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][(offset+t)>>20], (offset+t) & MMU.MMU_MASK[ARMCPU_ARM9][(offset+t)>>20]);
					T1WriteByte(MMU.MMU_MEM[ARMCPU_ARM9][(hi+t)>>20], (hi+t) & MMU.MMU_MASK[ARMCPU_ARM9][(hi+t)>>20], tmp);
				}
			break;
			//default: INFO("AR: ERROR uknown command 0x%2X at %08X:%08X\n", type, hi, lo); break;
		}
	}
}

BOOL CHEATS::XXcodePreParser(CHEATS_LIST *list, char *code)
{
	int		count = 0;
	u16		t = 0;
	char	tmp_buf[sizeof(list->code)];
	memset(tmp_buf, 0, sizeof(tmp_buf));

	size_t code_len = strlen(code);
	// remove wrong chars
	for (size_t i=0; i < code_len; i++)
	{
		char c = code[i];
		//apparently 100% of pokemon codes were typed with the letter O in place of zero in some places
		//so let's try to adjust for that here
		static const char *AR_Valid = "Oo0123456789ABCDEFabcdef";
		if (strchr(AR_Valid, c))
		{
			if(c=='o' || c=='O') c='0';
			tmp_buf[t++] = c;
		}
	}

	size_t len = strlen(tmp_buf);
	if ((len % 8) != 0) return FALSE;			// error
	if ((len % 16) != 0) return FALSE;			// error

	// TODO: syntax check
	count = (len / 16);
	for (int i=0; i < count; i++)
	{
		char buf[9] = {0};
		memcpy(buf, tmp_buf+(i*16), 8);
		sscanf(buf, "%x", &list->code[i][0]);
		memcpy(buf, tmp_buf+(i*16) + 8, 8);
		sscanf(buf, "%x", &list->code[i][1]);
	}
	
	list->num = count;
	list->size = 0;
	return TRUE;
}

BOOL CHEATS::add_AR_Direct(CHEATS_LIST cheat)
{
	size_t num = list.size();
	list.push_back(cheat);
	list[num].type = 1;
	return TRUE;
}

BOOL CHEATS::add_AR(char *code, char *description, BOOL enabled)
{
	//if (num == MAX_CHEAT_LIST) return FALSE;
	size_t num = list.size();

	CHEATS_LIST temp;
	if (!XXcodePreParser(&temp, code)) return FALSE;

	list.push_back(temp);
	
	list[num].type = 1;
	
	strcpy(list[num].description, description);
	list[num].enabled = enabled;
	return TRUE;
}

BOOL CHEATS::update_AR(char *code, char *description, BOOL enabled, u32 pos)
{
	if (pos >= list.size()) return FALSE;

	if (code != NULL)
	{
		if (!XXcodePreParser(&list[pos], code)) return FALSE;
		strcpy(list[pos].description, description);
		list[pos].type = 1;
	}
	
	list[pos].enabled = enabled;
	return TRUE;
}

BOOL CHEATS::add_CB(char *code, char *description, BOOL enabled)
{
	//if (num == MAX_CHEAT_LIST) return FALSE;
	size_t num = list.size();

	if (!XXcodePreParser(&list[num], code)) return FALSE;
	
	list[num].type = 2;
	
	strcpy(list[num].description, description);
	list[num].enabled = enabled;
	return TRUE;
}

BOOL CHEATS::update_CB(char *code, char *description, BOOL enabled, u32 pos)
{
	if (pos >= list.size()) return FALSE;

	if (code != NULL)
	{
		if (!XXcodePreParser(&list[pos], code)) return FALSE;
		list[pos].type = 2;
		strcpy(list[pos].description, description);
	}
	list[pos].enabled = enabled;
	return TRUE;
}

BOOL CHEATS::remove(u32 pos)
{
	if (pos >= list.size()) return FALSE;
	if (list.size() == 0) return FALSE;

	list.erase(list.begin()+pos);

	return TRUE;
}

void CHEATS::getListReset()
{
	currentGet = 0;
	return;
}

BOOL CHEATS::getList(CHEATS_LIST *cheat)
{
	if (currentGet >= list.size()) 
	{
		currentGet = 0;
		return FALSE;
	}
	//memcpy(cheat, &list[currentGet++], sizeof(CHEATS_LIST));
	*cheat = list[currentGet++];
	if (currentGet > list.size()) 
	{
		currentGet = 0;
		return FALSE;
	}
	return TRUE;
}

BOOL CHEATS::get(CHEATS_LIST *cheat, u32 pos)
{
	if (pos >= list.size()) return FALSE;
	*cheat = list[pos];
	return TRUE;
}

u32	CHEATS::getSize()
{
	return list.size();
}

BOOL CHEATS::save()
{
	const char	*types[] = {"DS", "AR", "CB"};
	char		buf[sizeof(list[0].code) * 2 + 200] = { 0 };
	FILE		*flist = fopen((char *)filename, "w");

	if (flist)
	{
		fprintf(flist, "; DeSmuME cheats file. VERSION %i.%03i\n", CHEAT_VERSION_MAJOR, CHEAT_VERSION_MINOR);
		std::wstring wtemp = (std::wstring)gameInfo.getRomBanner().titles[1];
		std::string temp = wcstombs(wtemp);
		strcpy(buf, temp.c_str());
		trim(buf);
		removeSpecialChars(buf);
		fprintf(flist, "Name=%s\n", buf);
		fprintf(flist, "Serial=%s\n", gameInfo.ROMserial);
		fputs("\n; cheats list\n", flist);
		for (size_t i = 0;  i < list.size(); i++)
		{
			if (list[i].num == 0) continue;
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "%s %c ", types[list[i].type], list[i].enabled?'1':'0');
			for (int t = 0; t < list[i].num; t++)
			{
				char buf2[10] = { 0 };

				u32 adr = list[i].code[t][0];
				if (list[i].type == 0)
				{
					//size of the cheat is written out as adr highest nybble
					adr &= 0x0FFFFFFF;
					adr |= (list[i].size << 28);
				}
				sprintf(buf2, "%08X", adr);
				strcat(buf, buf2);
				
				sprintf(buf2, "%08X", list[i].code[t][1]);
				strcat(buf, buf2);
				if (t < (list[i].num - 1))
					strcat(buf, ",");
			}
			strcat(buf, " ;");
			strcat(buf, trim(list[i].description));
			fprintf(flist, "%s\n", buf);
		}
		fputs("\n", flist);
		fclose(flist);
		return TRUE;
	}

	return FALSE;
}

char *CHEATS::clearCode(char *s)
{
	char	*buf = s;
	if (!s) return NULL;
	if (!*s) return s;

	for (u32 i = 0; i < strlen(s); i++)
	{
		if (s[i] == ';') break;
		if (strchr(hexValid, s[i]))
		{
			*buf = s[i];
			buf++;
		}
	}
	*buf = 0;
	return s;
}

BOOL CHEATS::load()
{
	FILE			*flist = fopen((char *)filename, "r");
	char			buf[sizeof(list[0].code) * 2 + 200] = { 0 };
	u32				last = 0;
	char			tmp_code[sizeof(list[0].code) * 2] = { 0 };
	u32				line = 0;

	if (flist)
	{
		INFO("Load cheats: %s\n", filename);
		clear();
		last = 0; line = 0;
		while (!feof(flist))
		{
			CHEATS_LIST		tmp_cht;
			line++;				// only for debug
			memset(buf, 0, sizeof(buf));
			if (fgets(buf, sizeof(buf), flist) == NULL) {
				//INFO("Cheats: Failed to read from flist at line %i\n", line);
				continue;
			}
			trim(buf);
			if ((strlen(buf) == 0) || (buf[0] == ';')) continue;
			if(!strncasecmp(buf,"name=",5)) continue;
			if(!strncasecmp(buf,"serial=",7)) continue;

			memset(&tmp_cht, 0, sizeof(tmp_cht));
			if ((buf[0] == 'D') && (buf[1] == 'S'))		// internal
				tmp_cht.type = 0;
			else
				if ((buf[0] == 'A') && (buf[1] == 'R'))	// Action Replay
					tmp_cht.type = 1;
				else
					if ((buf[0] == 'B') && (buf[1] == 'S'))	// Codebreaker
						tmp_cht.type = 2;
					else
						continue;
			// TODO: CB not supported
			if (tmp_cht.type == 3)
			{
				INFO("Cheats: Codebreaker code no supported at line %i\n", line);
				continue;
			}
			
			memset(tmp_code, 0, sizeof(tmp_code));
			strcpy(tmp_code, (char*)(buf+5));
			strcpy(tmp_code, clearCode(tmp_code));
			if ((strlen(tmp_code) == 0) || (strlen(tmp_code) % 16 !=0))
			{
				INFO("Cheats: Syntax error at line %i\n", line);
				continue;
			}

			tmp_cht.enabled = (buf[3] == '0')?FALSE:TRUE;
			u32 descr_pos = (u32)(std::max<s32>(strchr((char*)buf, ';') - buf, 0));
			if (descr_pos != 0)
				strcpy(tmp_cht.description, (buf + descr_pos + 1));

			tmp_cht.num = strlen(tmp_code) / 16;
			if ((tmp_cht.type == 0) && (tmp_cht.num > 1))
			{
				INFO("Cheats: Too many values for internal cheat\n", line);
				continue;
			}
			for (int i = 0; i < tmp_cht.num; i++)
			{
				char tmp_buf[9] = {0};

				strncpy(tmp_buf, (char*)(tmp_code + (i*16)), 8);
				sscanf_s(tmp_buf, "%x", &tmp_cht.code[i][0]);

				if (tmp_cht.type == 0)
				{
					tmp_cht.size = std::min<u32>(3, ((tmp_cht.code[i][0] & 0xF0000000) >> 28));
					tmp_cht.code[i][0] &= 0x00FFFFFF;
				}
				
				strncpy(tmp_buf, (char*)(tmp_code + (i*16) + 8), 8);
				sscanf_s(tmp_buf, "%x", &tmp_cht.code[i][1]);
			}

			list.push_back(tmp_cht);
			last++;
		}

		fclose(flist);
		INFO("Added %i cheat codes\n", list.size());
		return TRUE;
	}
	return FALSE;
}

void CHEATS::process()
{
	if (CommonSettings.cheatsDisable) return;
	if (list.size() == 0) return;
	size_t num = list.size();
	for (size_t i = 0; i < num; i++)
	{
		if (!list[i].enabled) continue;

		switch (list[i].type)
		{
			case 0:		// internal list system
				//INFO("list at 0x02|%06X value %i (size %i)\n",list[i].code[0], list[i].lo[0], list[i].size);
				switch (list[i].size)
				{
					case 0: T1WriteByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], list[i].code[0][0], list[i].code[0][1]); break;
					case 1: T1WriteWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], list[i].code[0][0], list[i].code[0][1]); break;
					case 2:
						{
							u32 tmp = T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], list[i].code[0][0]);
							tmp &= 0xFF000000;
							tmp |= (list[i].code[0][1] & 0x00FFFFFF);
							T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], list[i].code[0][0], tmp); 
							break;
						}
					case 3: T1WriteLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], list[i].code[0][0], list[i].code[0][1]); break;
				}
			break;

			case 1:		// Action Replay
				ARparser(list[i]);
				break;
			case 2:		// Codebreaker
				break;
			default: continue;
		}
	}
}

void CHEATS::getXXcodeString(CHEATS_LIST list, char *res_buf)
{
	char	buf[50] = { 0 };

	for (int i=0; i < list.num; i++)
	{
		sprintf(buf, "%08X %08X\n", list.code[i][0], list.code[i][1]);
		strcat(res_buf, buf);
	}
}

// ========================================== search
BOOL CHEATSEARCH::start(u8 type, u8 size, u8 sign)
{
	if (statMem) return FALSE;
	if (mem) return FALSE;

	statMem = new u8 [ ( 4 * 1024 * 1024 ) / 8 ];
	
	memset(statMem, 0xFF, ( 4 * 1024 * 1024 ) / 8);

	if (type == 1) // comparative search type (need 8Mb RAM !!! (4+4))
	{
		mem = new u8 [ ( 4 * 1024 * 1024 ) ];
		memcpy(mem, MMU.MMU_MEM[0][0x20], ( 4 * 1024 * 1024 ) );
	}

	_type = type;
	_size = size;
	_sign = sign;
	amount = 0;
	lastRecord = 0;
	
	//INFO("Cheat search system is inited (type %s)\n", type?"comparative":"exact");
	return TRUE;
}

BOOL CHEATSEARCH::close()
{
	if (statMem)
	{
		delete [] statMem;
		statMem = NULL;
	}

	if (mem)
	{
		delete [] mem;
		mem = NULL;
	}
	amount = 0;
	lastRecord = 0;
	//INFO("Cheat search system is closed\n");
	return FALSE;
}

u32 CHEATSEARCH::search(u32 val)
{
	amount = 0;

	switch (_size)
	{
		case 0:		// 1 byte
			for (u32 i = 0; i < (4 * 1024 * 1024); i++)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (1<<offs))
				{
					if ( T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) == val )
					{
						statMem[addr] |= (1<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(1<<offs);
				}
			}
		break;

		case 1:		// 2 bytes
			for (u32 i = 0; i < (4 * 1024 * 1024); i+=2)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (3<<offs))
				{
					if ( T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) == val )
					{
						statMem[addr] |= (3<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(3<<offs);
				}
			}
		break;

		case 2:		// 3 bytes
			for (u32 i = 0; i < (4 * 1024 * 1024); i+=3)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (0x7<<offs))
				{
					if ( (T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) & 0x00FFFFFF) == val )
					{
						statMem[addr] |= (0x7<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(0x7<<offs);
				}
			}
		break;

		case 3:		// 4 bytes
			for (u32 i = 0; i < (4 * 1024 * 1024); i+=4)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (0xF<<offs))
				{
					if ( T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) == val )
					{
						statMem[addr] |= (0xF<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(0xF<<offs);
				}
			}
		break;
	}

	return (amount);
}

u32 CHEATSEARCH::search(u8 comp)
{
	BOOL	res = FALSE;

	amount = 0;
	
	switch (_size)
	{
		case 0:		// 1 byte
			for (u32 i = 0; i < (4 * 1024 * 1024); i++)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (1<<offs))
				{
					switch (comp)
					{
						case 0: res=(T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) > T1ReadByte(mem, i)); break;
						case 1: res=(T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) < T1ReadByte(mem, i)); break;
						case 2: res=(T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) == T1ReadByte(mem, i)); break;
						case 3: res=(T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) != T1ReadByte(mem, i)); break;
						default: res = FALSE; break;
					}
					if ( res )
					{
						statMem[addr] |= (1<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(1<<offs);
				}
			}
		break;

		case 1:		// 2 bytes
			for (u32 i = 0; i < (4 * 1024 * 1024); i+=2)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (3<<offs))
				{
					switch (comp)
					{
						case 0: res=(T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) > T1ReadWord(mem, i)); break;
						case 1: res=(T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) < T1ReadWord(mem, i)); break;
						case 2: res=(T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) == T1ReadWord(mem, i)); break;
						case 3: res=(T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) != T1ReadWord(mem, i)); break;
						default: res = FALSE; break;
					}
					if ( res )
					{
						statMem[addr] |= (3<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(3<<offs);
				}
			}
		break;

		case 2:		// 3 bytes
			for (u32 i = 0; i < (4 * 1024 * 1024); i+=3)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (7<<offs))
				{
					switch (comp)
					{
						case 0: res=((T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) & 0x00FFFFFF) > (T1ReadLong(mem, i) & 0x00FFFFFF) ); break;
						case 1: res=((T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) & 0x00FFFFFF) < (T1ReadLong(mem, i) & 0x00FFFFFF) ); break;
						case 2: res=((T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) & 0x00FFFFFF) == (T1ReadLong(mem, i) & 0x00FFFFFF) ); break;
						case 3: res=((T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) & 0x00FFFFFF) != (T1ReadLong(mem, i) & 0x00FFFFFF) ); break;
						default: res = FALSE; break;
					}
					if ( res )
					{
						statMem[addr] |= (7<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(7<<offs);
				}
			}
		break;

		case 3:		// 4 bytes
			for (u32 i = 0; i < (4 * 1024 * 1024); i+=4)
			{
				u32	addr = (i >> 3);
				u32	offs = (i % 8);
				if (statMem[addr] & (0xF<<offs))
				{
					switch (comp)
					{
						case 0: res=(T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) > T1ReadLong(mem, i)); break;
						case 1: res=(T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) < T1ReadLong(mem, i)); break;
						case 2: res=(T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) == T1ReadLong(mem, i)); break;
						case 3: res=(T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) != T1ReadLong(mem, i)); break;
						default: res = FALSE; break;
					}
					if ( res )
					{
						statMem[addr] |= (0xF<<offs);
						amount++;
						continue;
					}
					statMem[addr] &= ~(0xF<<offs);
				}
			}
		break;
	}

	memcpy(mem, MMU.MMU_MEM[0][0x20], ( 4 * 1024 * 1024 ) );

	return (amount);
}

u32 CHEATSEARCH::getAmount()
{
	return (amount);
}

BOOL CHEATSEARCH::getList(u32 *address, u32 *curVal)
{
	u8	step = (_size+1);
	u8	stepMem = 1;
	switch (_size)
	{
		case 1: stepMem = 0x3; break;
		case 2: stepMem = 0x7; break;
		case 3: stepMem = 0xF; break;
	}

	for (u32 i = lastRecord; i < (4 * 1024 * 1024); i+=step)
	{
		u32	addr = (i >> 3);
		u32	offs = (i % 8);
		if (statMem[addr] & (stepMem<<offs))
		{
			*address = i;
			lastRecord = i+step;
			
			switch (_size)
			{
				case 0: *curVal=(u32)T1ReadByte(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i); return TRUE;
				case 1: *curVal=(u32)T1ReadWord(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i); return TRUE;
				case 2: *curVal=(u32)T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i) & 0x00FFFFFF; return TRUE;
				case 3: *curVal=(u32)T1ReadLong(MMU.MMU_MEM[ARMCPU_ARM9][0x20], i); return TRUE;
				default: return TRUE;
			}
		}
	}
	lastRecord = 0;
	return FALSE;
}

void CHEATSEARCH::getListReset()
{
	lastRecord = 0;
}

// ========================================================================= Export
bool CHEATSEXPORT::load(char *path)
{
	fp = fopen(path, "rb");
	if (!fp)
	{
		printf("Error open database\n");
		return false;
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (!search())
	{
		printf("ERROR: cheat in database not founded\n");
		return false;
	}
	
	if (!getCodes())
	{
		printf("ERROR: export cheats failed\n");
		return false;
	}

	return true;
}
void CHEATSEXPORT::close()
{
	if (fp)
		fclose(fp);
	if (cheats)
	{
		delete [] cheats;
		cheats = NULL;
	}
}

// TODO: decrypting database
bool CHEATSEXPORT::search()
{
	if (!fp) return false;

	u32 pos = 0x0100;
	FAT_R4	fat_empty;

	memset(&fat, 0, sizeof(FAT_R4));
	memset(&fat_empty, 0, sizeof(FAT_R4));

	fseek(fp, pos, SEEK_SET);
	while (pos < fsize)
	{
		fread(&fat, sizeof(FAT_R4), 1, fp);
		if (memcmp(&fat, &fat_empty, sizeof(FAT_R4)) == 0) break;
		if (memcmp(gameInfo.header.gameCode, &fat.serial[0], 4) == 0)
		{
			FAT_R4	fat_tmp;

			memset(&fat_tmp, 0, sizeof(FAT_R4));
			fread(&fat_tmp, sizeof(FAT_R4), 1, fp);
			if (memcmp(&fat_tmp, &fat_empty, sizeof(FAT_R4)) == 0)
			{
				// TODO
				dataSize = 0;
			}
			else
			{
				dataSize = (fat_tmp.addr - fat.addr);
			}
			char buf[5] = {0};
			memcpy(&buf, &fat.serial[0], 4);
			printf("Found %s CRC %08X at 0x%08llX (size %i)\n", buf, fat.CRC, fat.addr, dataSize);
			return true;
		}
		pos += sizeof(FAT_R4);
	}

	memset(&fat, 0, sizeof(FAT_R4));
	return false;
}

bool CHEATSEXPORT::getCodes()
{
	if (!fp) return false;

	u32	pos = 0;
	u32	pos_cht = 0;

	u8 *data = new u8 [dataSize+8];
	if (!data) return false;
	memset(data, 0, dataSize+8);
	
	fseek(fp, fat.addr, SEEK_SET);

	if (fread(data, 1, dataSize, fp) != dataSize)
	{
		delete [] data;
		data = NULL;
		return false;
	}

	u8 *title = data;
	u32 *cmd = (u32 *)(((intptr_t)title + strlen((char*)title) + 4) & 0xFFFFFFFC);
	numCheats = (*cmd) & (~0xF0000000);
	cmd += 9;
	cheats = new CHEATS_LIST[numCheats];
	memset(cheats, 0, sizeof(CHEATS_LIST) * numCheats);

	while (pos < numCheats)
	{
		u32 folderNum = 1;
		u8	*folderName = NULL;
		u8	*folderNote = NULL;
		if ((*cmd & 0xF0000000) == 0x10000000)	// Folder
		{
			folderNum = (*cmd  & 0x00FFFFFF);
			folderName = (u8*)((intptr_t)cmd + 4);
			folderNote = (u8*)((intptr_t)folderName + strlen((char*)folderName) + 1);
			pos++;
			numCheats--;
			cmd = (u32 *)(((intptr_t)folderName + strlen((char*)folderName) + 1 + strlen((char*)folderNote) + 1 + 3) & 0xFFFFFFFC);
		}

		for (int i = 0; i < folderNum; i++)		// in folder
		{
			u8 *cheatName = (u8 *)((intptr_t)cmd + 4);
			u8 *cheatNote = (u8 *)((intptr_t)cheatName + strlen((char*)cheatName) + 1);
			u32 *cheatData = (u32 *)(((intptr_t)cheatNote + strlen((char*)cheatNote) + 1 + 3) & 0xFFFFFFFC);
			u32 cheatDataLen = *cheatData++;

			if (cheatDataLen)
			{
				char buf[2048];
				memset(buf, 0, 2048);

				cheats[pos_cht].num = cheatDataLen/2;
				cheats[pos_cht].type = 1;

				if ( folderName && *folderName )
				{
					sprintf(cheats[pos_cht].description, "%s: ", folderName);
				}
				strcat(cheats[pos_cht].description, (char*)cheatName);
				
				if ( cheatNote && *cheatNote )
				{
					strcat(cheats[pos_cht].description, "| ");
					strcat(cheats[pos_cht].description, (char*)cheatNote);
				}

				for(u32 j = 0, t = 0; j < (cheatDataLen/2); j++, t+=2 )
				{
					cheats[pos_cht].code[j][0] = (u32)*(cheatData+t);
					//printf("%i: %08X ", j, cheats[pos_cht].code[j][0]);
					cheats[pos_cht].code[j][1] = (u32)*(cheatData+t+1);
					//printf("%08X\n", cheats[pos_cht].code[j][1]);
					
				}
			}
			pos++; pos_cht++;
			cmd = (u32 *)((intptr_t)cmd + ((*cmd + 1)*4));
		}
		
	};

	delete [] data;

	//for (int i = 0; i < numCheats; i++)
	//	printf("%i: %s\n", i, cheats[i].description);
	
	return true;
}

CHEATS_LIST *CHEATSEXPORT::getCheats()
{
	return cheats;
}
u32 CHEATSEXPORT::getCheatsNum()
{
	return numCheats;
}
