/*  windriver.h

    Copyright (C) 2008-2009 DeSmuME team

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

#ifndef _WINDRIVER_H_
#define _WINDRIVER_H_

#define WIN32_LEAN_AND_MEAN
#include "../common.h"
#include "CWindow.h"

extern WINCLASS	*MainWindow;

class Lock {
public:
	Lock(); // defaults to the critical section around NDS_exec
	Lock(CRITICAL_SECTION& cs);
	~Lock();
private:
	CRITICAL_SECTION* m_cs;
};

#endif