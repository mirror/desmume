/*  soundView.cpp

	Copyright (C) 2009-2010 DeSmuME team

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "SPU.h"
#include "xsfui.rh"
#include "NDSSystem.h"
#include <algorithm>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "soundView.h"

#include <assert.h>

using namespace std;

struct TCommonSettings {
	TCommonSettings() 
		: spu_captureMuted(false)
		, spu_advanced(false)
	{
		for(int i=0;i<16;i++)
			spu_muteChannels[i] = false;
	}
	bool spu_muteChannels[16];
	bool spu_advanced, spu_captureMuted;
	
} CommonSettings;



//////////////////////////////////////////////////////////////////////////////

typedef struct SoundView_DataStruct
{
	SoundView_DataStruct() 
		: viewFirst8Channels(TRUE)
		, volModeAlternate(FALSE)
		, destroyed(false)
	{
	}

	volatile bool destroyed;

	HWND hDlg;

	BOOL viewFirst8Channels;
	BOOL volModeAlternate;
} SoundView_DataStruct;

SoundView_DataStruct * SoundView_Data = NULL;


bool IsDlgCheckboxChecked(HWND hDlg, int id)
{
	return IsDlgButtonChecked(hDlg,id) == BST_CHECKED;
}

void CheckDlgItem(HWND hDlg, int id, bool checked)
{
	CheckDlgButton(hDlg, id, checked ? BST_CHECKED : BST_UNCHECKED);
}



//////////////////////////////////////////////////////////////////////////////

BOOL SoundView_Init()
{
	return 1;
}

void SoundView_DeInit()
{
}

//////////////////////////////////////////////////////////////////////////////

inline int chanOfs()
{
	return SoundView_Data->viewFirst8Channels ? 0 : 8;
}

extern "C" int SoundView_DlgOpen(HINSTANCE hAppInst)
{
	HWND hDlg;

	SoundView_Data = new SoundView_DataStruct();
	if(SoundView_Data == NULL)
		return 0;

	hDlg = CreateDialogParamW(hAppInst, MAKEINTRESOURCEW(IDD_SOUND_VIEW), GetDesktopWindow(), SoundView_DlgProc, (LPARAM)SoundView_Data);
	if(hDlg == NULL)
	{
		delete SoundView_Data;
		SoundView_Data = NULL;
		return 0;
	}

	SoundView_Data->hDlg = hDlg;

	ShowWindow(hDlg, SW_SHOW);
	UpdateWindow(hDlg);

	return 1;
}

extern "C" void SoundView_DlgClose()
{
	if(SoundView_Data != NULL )
	{
		SendMessage(SoundView_Data->hDlg,WM_APP,0,0);
		while(!SoundView_Data->destroyed)
			Sleep(1);
		delete SoundView_Data;
		SoundView_Data = NULL;
	}
}

BOOL SoundView_IsOpened()
{
	return (SoundView_Data != NULL);
}

HWND SoundView_GetHWnd()
{
	return SoundView_Data ? SoundView_Data->hDlg : NULL;
}

extern "C" void SoundView_Refresh()
{
	if(SoundView_Data == NULL || SPU_core == NULL)
		return;
	HWND hDlg = SoundView_Data->hDlg;

	PostMessage(hDlg,WM_APP+1,0,0);
}

void DoRefresh()
{
	if(SoundView_Data == NULL || SPU_core == NULL)
		return;
	HWND hDlg = SoundView_Data->hDlg;

	char buf[256];
	static const int format_shift[] = { 2, 1, 3, 0 };
	static const double ARM7_CLOCK = 33513982;
	for(int chanId = 0; chanId < 8; chanId++) {
		int chan = chanId + chanOfs();
		channel_struct &thischan = SPU_core->channels[chan];

		SendDlgItemMessage(hDlg, IDC_SOUND0PANBAR+chanId, PBM_SETPOS, (WPARAM)spumuldiv7(128, thischan.pan), (LPARAM)0);
		if(thischan.status != CHANSTAT_STOPPED)
		{
			s32 vol = spumuldiv7(128, thischan.vol) >> thischan.datashift;
			SendDlgItemMessage(hDlg, IDC_SOUND0VOLBAR+chanId, PBM_SETPOS,
				(WPARAM)vol, (LPARAM)0);

			if(SoundView_Data->volModeAlternate) 
				sprintf(buf, "%d/%d", thischan.vol, 1 << thischan.datashift);
			else
				sprintf(buf, "%d", vol);
			SetDlgItemText(hDlg, IDC_SOUND0VOL+chanId, buf);

			if (thischan.pan == 0)
				strcpy(buf, "L");
			else if (thischan.pan == 64)
				strcpy(buf, "C");
			else if (thischan.pan == 127)
				strcpy(buf, "R");
			else if (thischan.pan < 64)
				sprintf(buf, "L%d", 64 - thischan.pan);
			else //if (thischan.pan > 64)
				sprintf(buf, "R%d", thischan.pan - 64);
			SetDlgItemText(hDlg, IDC_SOUND0PAN+chanId, buf);

			sprintf(buf, "%d", thischan.hold);
			SetDlgItemText(hDlg, IDC_SOUND0HOLD+chanId, buf);

			sprintf(buf, "%d", thischan.status);
			SetDlgItemText(hDlg, IDC_SOUND0BUSY+chanId, buf);

			const char* modes[] = { "Manual", "Loop Infinite", "One-Shot", "Prohibited" };
			sprintf(buf, "%d (%s)", thischan.repeat, modes[thischan.repeat]);
			SetDlgItemText(hDlg, IDC_SOUND0REPEATMODE+chanId, buf);

			if(thischan.format != 3) {
				const char* formats[] = { "PCM8", "PCM16", "IMA-ADPCM" };
				sprintf(buf, "%d (%s)", thischan.format, formats[thischan.format]);
				SetDlgItemText(hDlg, IDC_SOUND0FORMAT+chanId, buf);
			}
			else {
				if (chan < 8)
					sprintf(buf, "%d (PSG/Noise?)", thischan.format);
				else if (chan < 14)
					sprintf(buf, "%d (%.1f% Square)", thischan.format, (float)thischan.waveduty/8);
				else
					sprintf(buf, "%d (Noise)", thischan.format);
			}

			sprintf(buf, "$%07X", thischan.addr);
			SetDlgItemText(hDlg, IDC_SOUND0SAD+chanId, buf);

			sprintf(buf, "samp #%d", thischan.loopstart << format_shift[thischan.format]);
			SetDlgItemText(hDlg, IDC_SOUND0PNT+chanId, buf);

			sprintf(buf, "$%04X (%.1f Hz)", thischan.timer, (ARM7_CLOCK/2) / (double)(0x10000 - thischan.timer));
			SetDlgItemText(hDlg, IDC_SOUND0TMR+chanId, buf);

			sprintf(buf, "samp #%d / #%d", sputrunc(thischan.sampcnt), thischan.totlength << format_shift[thischan.format]);
			SetDlgItemText(hDlg, IDC_SOUND0POSLEN+chanId, buf);
		}
		else {
			SendDlgItemMessage(hDlg, IDC_SOUND0VOLBAR+chanId, PBM_SETPOS, (WPARAM)0, (LPARAM)0);
			strcpy(buf, "---");
			SetDlgItemText(hDlg, IDC_SOUND0VOL+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0PAN+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0HOLD+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0BUSY+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0REPEATMODE+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0FORMAT+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0SAD+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0PNT+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0TMR+chanId, buf);
			SetDlgItemText(hDlg, IDC_SOUND0POSLEN+chanId, buf);
		}
	} //chan loop

	//TODO - CAP REGS
	////ctrl
	//{
	//	CheckDlgItem(hDlg,IDC_SNDCTRL_ENABLE,SPU_core->regs.masteren!=0);
	//	CheckDlgItem(hDlg,IDC_SNDCTRL_CH1NOMIX,SPU_core->regs.ctl_ch1bypass!=0);
	//	CheckDlgItem(hDlg,IDC_SNDCTRL_CH3NOMIX,SPU_core->regs.ctl_ch3bypass!=0);

	//	sprintf(buf,"%04X",_MMU_ARM7_read16(0x04000500));
	//	SetDlgItemText(hDlg,IDC_SNDCTRL_CTRL,buf);

	//	sprintf(buf,"%04X",_MMU_ARM7_read16(0x04000504));
	//	SetDlgItemText(hDlg,IDC_SNDCTRL_BIAS,buf);

	//	sprintf(buf,"%02X",SPU_core->regs.mastervol);
	//	SetDlgItemText(hDlg,IDC_SNDCTRL_VOL,buf);

	//	sprintf(buf,"%01X",SPU_core->regs.ctl_left);
	//	SetDlgItemText(hDlg,IDC_SNDCTRL_LEFTOUT,buf);

	//	sprintf(buf,"%01X",SPU_core->regs.ctl_right);
	//	SetDlgItemText(hDlg,IDC_SNDCTRL_RIGHTOUT,buf);

	//	static const char* leftouttext[] = {"L-Mix","Ch1","Ch3","Ch1+3"};
	//	static const char* rightouttext[] = {"R-Mix","Ch1","Ch3","Ch1+3"};

	//	SetDlgItemText(hDlg,IDC_SNDCTRL_LEFTOUTTEXT,leftouttext[SPU_core->regs.ctl_left]);

	//	SetDlgItemText(hDlg,IDC_SNDCTRL_RIGHTOUTTEXT,rightouttext[SPU_core->regs.ctl_right]);

	//}

	////cap0
	//{
	//	SPU_struct::REGS::CAP& cap = SPU_core->regs.cap[0];

	//	CheckDlgItem(hDlg,IDC_CAP0_ADD,cap.add!=0);
	//	CheckDlgItem(hDlg,IDC_CAP0_SRC,cap.source!=0);
	//	CheckDlgItem(hDlg,IDC_CAP0_ONESHOT,cap.oneshot!=0);
	//	CheckDlgItem(hDlg,IDC_CAP0_TYPE,cap.bits8!=0);
	//	CheckDlgItem(hDlg,IDC_CAP0_ACTIVE,cap.active!=0);
	//	CheckDlgItem(hDlg,IDC_CAP0_RUNNING,cap.runtime.running!=0);

	//	if(cap.source) SetDlgItemText(hDlg,IDC_CAP0_SRCTEXT,"Ch2");
	//	else SetDlgItemText(hDlg,IDC_CAP0_SRCTEXT,"L-Mix");

	//	if(cap.bits8) SetDlgItemText(hDlg,IDC_CAP0_TYPETEXT,"Pcm8");
	//	else SetDlgItemText(hDlg,IDC_CAP0_TYPETEXT,"Pcm16");

	//	sprintf(buf,"%02X",_MMU_ARM7_read08(0x04000508));
	//	SetDlgItemText(hDlg,IDC_CAP0_CTRL,buf);

	//	sprintf(buf,"%08X",cap.dad);
	//	SetDlgItemText(hDlg,IDC_CAP0_DAD,buf);

	//	sprintf(buf,"%08X",cap.len);
	//	SetDlgItemText(hDlg,IDC_CAP0_LEN,buf);

	//	sprintf(buf,"%08X",cap.runtime.curdad);
	//	SetDlgItemText(hDlg,IDC_CAP0_CURDAD,buf);
	//}

	////cap1
	//{
	//	SPU_struct::REGS::CAP& cap = SPU_core->regs.cap[1];

	//	CheckDlgItem(hDlg,IDC_CAP1_ADD,cap.add!=0);
	//	CheckDlgItem(hDlg,IDC_CAP1_SRC,cap.source!=0);
	//	CheckDlgItem(hDlg,IDC_CAP1_ONESHOT,cap.oneshot!=0);
	//	CheckDlgItem(hDlg,IDC_CAP1_TYPE,cap.bits8!=0);
	//	CheckDlgItem(hDlg,IDC_CAP1_ACTIVE,cap.active!=0);
	//	CheckDlgItem(hDlg,IDC_CAP1_RUNNING,cap.runtime.running!=0);

	//	if(cap.source) SetDlgItemText(hDlg,IDC_CAP1_SRCTEXT,"Ch3"); //maybe call it "Ch3(+2)" if it fits
	//	else SetDlgItemText(hDlg,IDC_CAP1_SRCTEXT,"R-Mix");

	//	if(cap.bits8) SetDlgItemText(hDlg,IDC_CAP1_TYPETEXT,"Pcm8");
	//	else SetDlgItemText(hDlg,IDC_CAP1_TYPETEXT,"Pcm16");

	//	sprintf(buf,"%02X",_MMU_ARM7_read08(0x04000509));
	//	SetDlgItemText(hDlg,IDC_CAP1_CTRL,buf);

	//	sprintf(buf,"%08X",cap.dad);
	//	SetDlgItemText(hDlg,IDC_CAP1_DAD,buf);

	//	sprintf(buf,"%08X",cap.len);
	//	SetDlgItemText(hDlg,IDC_CAP1_LEN,buf);

	//	sprintf(buf,"%08X",cap.runtime.curdad);
	//	SetDlgItemText(hDlg,IDC_CAP1_CURDAD,buf);
	//}
}

//////////////////////////////////////////////////////////////////////////////


static void updateMute_toSettings(HWND hDlg, int chan)
{
	//for(int chanId = 0; chanId < 8; chanId++)
	//	CommonSettings.spu_muteChannels[chanId+chanOfs()] = IsDlgButtonChecked(hDlg, IDC_SOUND0MUTE+chanId) == BST_CHECKED;
}

static void updateMute_allFromSettings(HWND hDlg)
{
	//for(int chanId = 0; chanId < 16; chanId++)
	//	CheckDlgItem(hDlg,IDC_SOUND0MUTE+chanId,CommonSettings.spu_muteChannels[chanId]);
}

static void updateMute_fromSettings(HWND hDlg)
{
	//for(int chanId = 0; chanId < 8; chanId++)
	//	CheckDlgItem(hDlg,IDC_SOUND0MUTE+chanId,CommonSettings.spu_muteChannels[chanId+chanOfs()]);
}
static void SoundView_SwitchChanOfs(SoundView_DataStruct *data)
{
	if (data == NULL)
		return;

	HWND hDlg = data->hDlg;
	data->viewFirst8Channels = !data->viewFirst8Channels;
	SetWindowText(GetDlgItem(hDlg, IDC_SOUNDVIEW_CHANSWITCH),
		data->viewFirst8Channels ? "V" : "^");

	char buf[256];
	for(int chanId = 0; chanId < 8; chanId++) {
		int chan = chanId + chanOfs();
		sprintf(buf, "#%02d", chan);
		SetDlgItemText(hDlg, IDC_SOUND0ID+chanId, buf);
	}

	updateMute_fromSettings(hDlg);
	CheckDlgItem(hDlg,IDC_SOUND_CAPTURE_MUTED,CommonSettings.spu_captureMuted);

	SoundView_Refresh();
}


static INT_PTR CALLBACK SoundView_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//SoundView_DataStruct *data = (SoundView_DataStruct*)GetWindowLongPtr(hDlg, DWLP_USER);
	//if((data == NULL) && (uMsg != WM_INITDIALOG))
	//	return 0;

	SoundView_DataStruct *data = SoundView_Data;

	switch(uMsg)
	{
	case WM_NCDESTROY:
		data->destroyed = true;
		break;
	case WM_APP+0:
		DestroyWindow(hDlg);
		break;
	case WM_APP+1:
		DoRefresh();
		break;
	case WM_INITDIALOG:
		{
			//for(int chanId = 0; chanId < 8; chanId++) {
			//	SendDlgItemMessage(hDlg, IDC_SOUND0VOLBAR+chanId, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 128));
			//	SendDlgItemMessage(hDlg, IDC_SOUND0PANBAR+chanId, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 128));
			//}

			//for(int chanId = 0; chanId < 8; chanId++) {
			//	if(CommonSettings.spu_muteChannels[chanId])
			//		SendDlgItemMessage(hDlg, IDC_SOUND0MUTE+chanId, BM_SETCHECK, TRUE, 0);
			//}

			//if(data == NULL)
			//{
			//	data = (SoundView_DataStruct*)lParam;
			//	SetWindowLongPtr(hDlg, DWLP_USER, (LONG)data);
			//}
			//data->hDlg = hDlg;

			//data->viewFirst8Channels = !data->viewFirst8Channels;
			//SoundView_SwitchChanOfs(data);
			////SoundView_Refresh();
			////InvalidateRect(hDlg, NULL, FALSE); UpdateWindow(hDlg);
		}
		return 1;

	case WM_CLOSE:
		SoundView_DlgClose();
		return 1;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			SoundView_DlgClose();
			return 1;
		case IDC_BUTTON_VOLMODE:
			data->volModeAlternate = IsDlgButtonChecked(hDlg, IDC_BUTTON_VOLMODE);
			return 1;

		case IDC_SOUND0MUTE+0:
		case IDC_SOUND0MUTE+1:
		case IDC_SOUND0MUTE+2:
		case IDC_SOUND0MUTE+3:
		case IDC_SOUND0MUTE+4:
		case IDC_SOUND0MUTE+5:
		case IDC_SOUND0MUTE+6:
		case IDC_SOUND0MUTE+7:
			updateMute_toSettings(hDlg,LOWORD(wParam)-IDC_SOUND0MUTE);
			return 1;
		case IDC_SOUND_CAPTURE_MUTED:
			CommonSettings.spu_captureMuted = IsDlgButtonChecked(hDlg,IDC_SOUND_CAPTURE_MUTED) != 0;
			return 1;
		case IDC_SOUND_UNMUTE_ALL:
			for(int i=0;i<16;i++) CommonSettings.spu_muteChannels[i] = false;
			updateMute_allFromSettings(hDlg);
			return 1;
		case IDC_SOUND_ANALYZE_CAP:
			printf("WTF\n");
			for(int i=0;i<16;i++) CommonSettings.spu_muteChannels[i] = true;
			CommonSettings.spu_muteChannels[1] = false;
			CommonSettings.spu_muteChannels[3] = false;
			CommonSettings.spu_captureMuted = true;
			updateMute_allFromSettings(hDlg);
			CheckDlgItem(hDlg,IDC_SOUND_CAPTURE_MUTED,CommonSettings.spu_captureMuted);
			return 1;


		case IDC_SOUNDVIEW_CHANSWITCH:
			{
				SoundView_SwitchChanOfs(data);
			}
			return 1;
		}
		return 0;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
