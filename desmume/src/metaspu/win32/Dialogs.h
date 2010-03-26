/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifdef _XBOX
#include <xtl.h>
#elif _WIN32
#include <windows.h>
#endif

#include "../SoundTouch/SoundTouch.h"

//#ifdef _WIN32
//#	include "WinConfig.h"
//#else
//#	include "LnxConfig.h"
//#endif
//
//namespace DebugConfig
//{
//	extern void ReadSettings();
//	extern void WriteSettings();
//	extern void OpenDialog();
//	extern void EnableControls( HWND hWnd );
//}
//
namespace SoundtouchCfg
{
	extern void ReadSettings();
	extern void WriteSettings();
	extern void OpenDialog( HWND hWnd );
	extern BOOL CALLBACK DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	extern void ApplySettings( soundtouch::SoundTouch& sndtouch );
}
//
//extern int		SendDialogMsg( HWND hwnd, int dlgId, UINT code, WPARAM wParam, LPARAM lParam);
//extern HRESULT	GUIDFromString( const char *str, LPGUID guid );
//
//extern void		AssignSliderValue( HWND idcwnd, HWND hwndDisplay, int value );
//extern void		AssignSliderValue( HWND hWnd, int idc, int editbox, int value );
//extern int		GetSliderValue( HWND hWnd, int idc );
//extern BOOL		DoHandleScrollMessage( HWND hwndDisplay, WPARAM wParam, LPARAM lParam );
//
//extern bool		CfgFindName( const TCHAR *Section, const TCHAR* Name);
//
//extern void		CfgWriteBool(const TCHAR* Section, const TCHAR* Name, bool Value);
//extern void		CfgWriteInt(const TCHAR* Section, const TCHAR* Name, int Value);
//extern void		CfgWriteStr(const TCHAR* Section, const TCHAR* Name, const wstring& Data);
//
//extern bool		CfgReadBool(const TCHAR *Section,const TCHAR* Name, bool Default);
//extern void		CfgReadStr(const TCHAR* Section, const TCHAR* Name, wstring& Data, int DataSize, const TCHAR* Default);
//extern void		CfgReadStr(const TCHAR* Section, const TCHAR* Name, TCHAR* Data, int DataSize, const TCHAR* Default);
//extern int		CfgReadInt(const TCHAR* Section, const TCHAR* Name,int Default);
//
//
//// Items Specific to DirectSound
//#define STRFY(x) #x
//#define verifyc(x) Verifyc(x,STRFY(x))
//
//extern void Verifyc(HRESULT hr, const char* fn);
//
//struct ds_device_data
//{
//	std::wstring name;
//	GUID guid;
//	bool hasGuid;
//};
//
