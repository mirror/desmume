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

//TODO - need to make a generic way for saving these configs and add in the windows
//rc configurator

#include "types.h"
//#include "Global.h"
#include "Dialogs.h"

#include "../metaspu.h"

#include "../SoundTouch/SoundTouch.h"

static int SequenceLenMS = 63;
static int SeekWindowMS = 16;
static int OverlapMS = 7;

// Timestretch Slider Bounds, Min/Max
static const int SequenceLen_Min = 50;
static const int SequenceLen_Max = 90;

static const int SeekWindow_Min = 10;
static const int SeekWindow_Max = 32;

static const int Overlap_Min = 3;
static const int Overlap_Max = 25;

void SoundtouchCfg::ApplySettings( soundtouch::SoundTouch& sndtouch )
{
	sndtouch.setSetting( SETTING_SEQUENCE_MS,	SequenceLenMS );
	sndtouch.setSetting( SETTING_SEEKWINDOW_MS,	SeekWindowMS );
	sndtouch.setSetting( SETTING_OVERLAP_MS,	OverlapMS );
}

static void ClampValues()
{
	Clampify( SequenceLenMS, SequenceLen_Min, SequenceLen_Max );
	Clampify( SeekWindowMS, SeekWindow_Min, SeekWindow_Max );
	Clampify( OverlapMS, Overlap_Min, Overlap_Max );
}

void SoundtouchCfg::ReadSettings()
{
/*	SequenceLenMS	= CfgReadInt( L"SOUNDTOUCH", L"SequenceLengthMS", 50 );
	SeekWindowMS	= CfgReadInt( L"SOUNDTOUCH", L"SeekWindowMS", 15 );
	OverlapMS		= CfgReadInt( L"SOUNDTOUCH", L"OverlapMS", 25 );

	ClampValues();	*/	
}

void SoundtouchCfg::WriteSettings()
{
	//CfgWriteInt( L"SOUNDTOUCH", L"SequenceLengthMS", SequenceLenMS );
	//CfgWriteInt( L"SOUNDTOUCH", L"SeekWindowMS", SeekWindowMS );
	//CfgWriteInt( L"SOUNDTOUCH", L"OverlapMS", OverlapMS );
}

//BOOL CALLBACK SoundtouchCfg::DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
//{
//	int wmId,wmEvent;
//	wchar_t temp[384]={0};
//
//	switch(uMsg)
//	{
//		case WM_PAINT:
//			return FALSE;
//
//		case WM_INITDIALOG:
//		{
//			INIT_SLIDER( IDC_SEQLEN_SLIDER, SequenceLen_Min, SequenceLen_Max, 20, 5, 1 );
//			INIT_SLIDER( IDC_SEEKWIN_SLIDER, SeekWindow_Min, SeekWindow_Max, 5, 2, 1 );
//			INIT_SLIDER( IDC_OVERLAP_SLIDER, Overlap_Min, Overlap_Max, 3, 2, 1 );
//
//			SendDialogMsg( hWnd, IDC_SEQLEN_SLIDER, TBM_SETPOS, TRUE, SequenceLenMS );
//			SendDialogMsg( hWnd, IDC_SEEKWIN_SLIDER, TBM_SETPOS, TRUE, SeekWindowMS );
//			SendDialogMsg( hWnd, IDC_OVERLAP_SLIDER, TBM_SETPOS, TRUE, OverlapMS );
//		}
//		
//		case WM_COMMAND:
//			wmId    = LOWORD(wParam); 
//			wmEvent = HIWORD(wParam); 
//			// Parse the menu selections:
//			if( wmId == IDOK )
//			{
//				SequenceLenMS	= (int)SendDialogMsg( hWnd, IDC_SEQLEN_SLIDER, TBM_GETPOS, 0, 0 );
//				SeekWindowMS	= (int)SendDialogMsg( hWnd, IDC_SEEKWIN_SLIDER, TBM_GETPOS, 0, 0 );
//				OverlapMS		= (int)SendDialogMsg( hWnd, IDC_OVERLAP_SLIDER, TBM_GETPOS, 0, 0 );
//
//				ClampValues();
//				WriteSettings();
//				EndDialog(hWnd,0);
//			}
//			else if( wmId == IDCANCEL )
//			{
//				EndDialog(hWnd,0);
//			}
//		break;
//		
//		case WM_HSCROLL:
//			DoHandleScrollMessage( hWnd, wParam, lParam );
//		break;
//		
//		default:
//			return FALSE;
//	}
//	return TRUE;
//}

void SoundtouchCfg::OpenDialog( HWND hWnd )
{
	//INT_PTR ret;
	//ret = DialogBox( hInstance, MAKEINTRESOURCE(IDD_CONFIG_SOUNDTOUCH), hWnd, (DLGPROC)DialogProc );
	//if(ret==-1)
	//{
	//	MessageBoxEx(GetActiveWindow(), L"Error Opening the Soundtouch advanced dialog.", L"OMG ERROR!", MB_OK, 0);
	//	return;
	//}
	//ReadSettings();
}
