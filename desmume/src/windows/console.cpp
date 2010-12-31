/*  Copyright 2008-2009 DeSmuME team

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

#include "../common.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "version.h"


///////////////////////////////////////////////////////////////// Console
#define BUFFER_SIZE 100
HANDLE hConsole = NULL;
void printlog(const char *fmt, ...);

void OpenConsole() 
{
	COORD csize = {0};
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo = {0}; 
	SMALL_RECT srect = {0};
	char buf[256] = {0};

	//dont do anything if we're already attached
	if (hConsole) return;

	//attach to an existing console (if we can; this is circuitous because AttachConsole wasnt added until XP)
	//remember to abstract this late bound function notion if we end up having to do this anywhere else
	bool attached = false;
	HMODULE lib = LoadLibrary("kernel32.dll");
	if(lib)
	{
		typedef BOOL (WINAPI *_TAttachConsole)(DWORD dwProcessId);
		_TAttachConsole _AttachConsole  = (_TAttachConsole)GetProcAddress(lib,"AttachConsole");
		if(_AttachConsole)
		{
			if(_AttachConsole(-1))
				attached = true;
		}
		FreeLibrary(lib);
	}

	//if we failed to attach, then alloc a new console
	if(!attached)
	{
		if (!AllocConsole()) return;
	}

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hConsole == INVALID_HANDLE_VALUE) return;
	//redirect stdio
	long lStdHandle = (long)hConsole;
	int hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	if(hConHandle == -1)
		return; //this fails from a visual studio command prompt
	
#if 1
	FILE *fp = _fdopen( hConHandle, "w" );
#else
	FILE *fp = fopen( "c:\\desmume.log", "w" );
#endif
	*stdout = *fp;
	//and stderr
	*stderr = *fp;

	sprintf(buf,"%s OUTPUT", EMU_DESMUME_NAME_AND_VERSION());
	SetConsoleTitle(TEXT(buf));
	csize.X = 60;
	csize.Y = 800;
	SetConsoleScreenBufferSize(hConsole, csize);
	GetConsoleScreenBufferInfo(hConsole, &csbiInfo);
	srect = csbiInfo.srWindow;
	srect.Right = srect.Left + 99;
	srect.Bottom = srect.Top + 64;
	SetConsoleWindowInfo(hConsole, TRUE, &srect);
	SetConsoleCP(GetACP());
	SetConsoleOutputCP(GetACP());
	if(attached) printlog("\n");
	printlog("%s\n",EMU_DESMUME_NAME_AND_VERSION());
	printlog("- compiled: %s %s\n\n",__DATE__,__TIME__);
}

void CloseConsole() {
	if (hConsole == NULL) return;
	printlog("Closing...");
	FreeConsole(); 
	hConsole = NULL;
}

void printlog(const char *fmt, ...)
{
	va_list list;
	char msg[512];
	DWORD tmp;

	memset(msg,0,512);

	va_start(list,fmt);
		_vsnprintf(msg,511,fmt,list);
	va_end(list);
	WriteConsole(hConsole,msg, (DWORD)strlen(msg), &tmp, 0);
}
