/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

    Copyright 2009 CrazyMax
	Copyright 2009 DeSmuME team

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

#include "cheatsWin.h"
#include <commctrl.h>
#include "../cheatSystem.h"
#include "resource.h"
#include "../debug.h"

static	u8		searchType = 0;
static	u8		searchSize = 0;
static	u8		searchSign = 0;
static	u8		searchStep = 0;
static	u8		searchComp = 0;
static	HWND	hBRestart = NULL;
static	HWND	hBView = NULL;
static	HWND	hBSearch = NULL;
static	u32		exactVal = 0;
static	u32		searchNumberResults = 0;

u8				searchAddProc = 0;
u32				searchAddAddress = 0;
u32				searchAddValue = 0;
u8				searchAddMode = 0;
u8				searchAddFreeze = 1;
u8				searchAddSize = 0;
static char		editBuf[4][75] = { 0 };

HWND			searchWnd = NULL;
HWND			searchListView = NULL;
HWND			cheatListView = NULL;
WNDPROC			oldEditProc = NULL;
WNDPROC			oldEditProcHEX = NULL;

CHEATS_LIST		tempCheat;

static	char *NAME_CPUs[2] = { "ARM9", "ARM7" };

u32		searchIDDs[2][4] = {
	{ IDD_CHEAT_SEARCH_MAIN, IDD_CHEAT_SEARCH_EXACT, IDD_CHEAT_SEARCH_RESULT, NULL },
	{ IDD_CHEAT_SEARCH_MAIN, IDD_CHEAT_SEARCH_RESULT, IDD_CHEAT_SEARCH_COMP, IDD_CHEAT_SEARCH_RESULT}
};

u32	searchSizeIDDs[4] = { IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4 };
u32	searchSignIDDs[2] = { IDC_RADIO5, IDC_RADIO6 };
u32	searchTypeIDDs[2] = { IDC_RADIO7, IDC_RADIO8 };
u32	searchCompIDDs[4] = { IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4 };

u32	searchRangeIDDs[4] = { IDC_STATIC_S1, IDC_STATIC_S2, IDC_STATIC_S3, IDC_STATIC_S4 };
char *searchRangeText[2][4] = { {"[0..255]", "[0..65536]", "[0..16777215]", "[0..4294967295]"},
								{"[-128..127]", "[-32168..32767]", "[-8388608..8388607]", "[-2147483648..2147483647]"}};

u32 searchRange[4][2] = { 
							{ 0, 255 },
							{ 0, 65536 },
							{ 0, 16777215 },
							{ 0, 4294967295 }
						};

LRESULT CALLBACK EditValueProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// TODO: check paste
	if (msg == WM_CHAR)
	{
		switch (wParam)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				break;
			case '-':
				{
					u8 pos = 0;
					SendMessage(hwnd, EM_GETSEL, (WPARAM)&pos, NULL);
					if (pos != 0) wParam = 0;
				}
				break;

			case VK_BACK:
				break;
			default:
				wParam = 0;
				break;
		}
        
	}

	return CallWindowProc(oldEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditValueHEXProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// TODO: check paste
	if (msg == WM_CHAR)
	{
		switch (wParam)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				wParam -= 32;
				break;
			case VK_BACK:
				break;
			default:
				wParam = 0;
				break;
		}
        
	}

	return CallWindowProc(oldEditProcHEX, hwnd, msg, wParam, lParam);
}

BOOL CALLBACK CheatsAddProc(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	static WNDPROC saveOldEditProc = NULL;

	switch(msg)
	{
		case WM_INITDIALOG:
			{	
				saveOldEditProc = oldEditProc;
				SendMessage(GetDlgItem(dialog, IDC_EDIT1), EM_SETLIMITTEXT, 6, 0);
				SendMessage(GetDlgItem(dialog, IDC_EDIT2), EM_SETLIMITTEXT, 10, 0);
				SendMessage(GetDlgItem(dialog, IDC_EDIT3), EM_SETLIMITTEXT, 75, 0);
				oldEditProcHEX = (WNDPROC)SetWindowLongPtr(GetDlgItem(dialog, IDC_EDIT1), GWLP_WNDPROC, (LONG)EditValueHEXProc);
				oldEditProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(dialog, IDC_EDIT2), GWLP_WNDPROC, (LONG)EditValueProc);

				if (searchAddMode == 1)
				{
					char buf[10];
					searchAddAddress &= 0x00FFFFFF;
					wsprintf(buf, "%06X", searchAddAddress);
					SetWindowText(GetDlgItem(dialog, IDC_EDIT1), buf);
					wsprintf(buf, "%i", searchAddValue);
					SetWindowText(GetDlgItem(dialog, IDC_EDIT2), buf);
					EnableWindow(GetDlgItem(dialog, IDOK), TRUE);
					if (searchAddProc)
						CheckDlgButton(dialog, IDC_RADIO9, BST_CHECKED);
					else
						CheckDlgButton(dialog, IDC_RADIO8, BST_CHECKED);
					EnableWindow(GetDlgItem(dialog, IDC_EDIT1), FALSE);
					EnableWindow(GetDlgItem(dialog, IDC_RADIO1), FALSE);
					EnableWindow(GetDlgItem(dialog, IDC_RADIO2), FALSE);
					EnableWindow(GetDlgItem(dialog, IDC_RADIO3), FALSE);
					EnableWindow(GetDlgItem(dialog, IDC_RADIO4), FALSE);
					EnableWindow(GetDlgItem(dialog, IDC_RADIO8), FALSE);
					EnableWindow(GetDlgItem(dialog, IDC_RADIO9), FALSE);
					ltoa(searchAddProc, editBuf[3], 10);
					strcpy(editBuf[3], "0");
				}
				else
				{
					SetWindowText(GetDlgItem(dialog, IDC_EDIT2), "0");
					CheckDlgButton(dialog, IDC_RADIO1, BST_CHECKED);
					strcpy(editBuf[3], "0");
				}

				memset(editBuf, 0, sizeof(editBuf));

				GetWindowText(GetDlgItem(dialog, IDC_EDIT1), editBuf[0], 10);
				GetWindowText(GetDlgItem(dialog, IDC_EDIT2), editBuf[1], 10);
				
				CheckDlgButton(dialog, IDC_CHECK1, BST_CHECKED);
				CheckDlgButton(dialog, searchSizeIDDs[searchAddSize], BST_CHECKED);
			}
		return TRUE;
		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDOK:
				{
					u32 tmp_addr = 0;
					sscanf_s(editBuf[0], "%x", &tmp_addr);

					oldEditProc = saveOldEditProc;
					if (cheatsAdd(atol(editBuf[3]), searchAddSize, tmp_addr, atol(editBuf[1]), editBuf[2], searchAddFreeze))
					{
						if (cheatsSave())
						{
							searchAddAddress = tmp_addr;
							searchAddValue = atol(editBuf[1]);

							EndDialog(dialog, TRUE);
						}
					}
				}
				return TRUE;

				case IDCANCEL:
					oldEditProc = saveOldEditProc;
					EndDialog(dialog, FALSE);
				return TRUE;

				case IDC_EDIT1:
				{
					if (HIWORD(wparam) == EN_UPDATE)
					{
						GetWindowText(GetDlgItem(dialog, IDC_EDIT1), editBuf[0], 8);
						if (!strlen(editBuf[0]) && !strlen(editBuf[1]))
						{
							EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
							return TRUE;
						}
						
						u32 val = 0;
						sscanf_s(editBuf[0], "%x", &val);
						val &= 0x00FFFFFF;
						if (val > 0x400000)
						{
							EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
							return TRUE;
						}
						EnableWindow(GetDlgItem(dialog, IDOK), TRUE);
					}
					return TRUE;
				}

				case IDC_EDIT2:
				{
					if (HIWORD(wparam) == EN_UPDATE)
					{
						GetWindowText(GetDlgItem(dialog, IDC_EDIT1), editBuf[1], 10);
						if (!strlen(editBuf[1]) && !strlen(editBuf[0]))
						{
							EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
							return TRUE;
						}
						
						u32 val = atol(editBuf[1]);
						if (val > searchRange[searchAddSize][1])
						{
							EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
							return TRUE;
						}
						EnableWindow(GetDlgItem(dialog, IDOK), TRUE);
					}
					return TRUE;
				}

				case IDC_EDIT3:
				{
					if (HIWORD(wparam) == EN_UPDATE)
						GetWindowText(GetDlgItem(dialog, IDC_EDIT3), editBuf[2], 75);
					return TRUE;
				}

				case IDC_RADIO8:
				{
					strcpy(editBuf[3], "0");
					return TRUE;
				}

				case IDC_RADIO9:
				{
					strcpy(editBuf[3], "1");
					return TRUE;
				}

				case IDC_RADIO1:		// 1 byte
					searchAddSize = 0;
				return TRUE;
				case IDC_RADIO2:		// 2 bytes
					searchAddSize = 1;
				return TRUE;
				case IDC_RADIO3:		// 3 bytes
					searchAddSize = 2;
				return TRUE;
				case IDC_RADIO4:		// 4 bytes
					searchAddSize = 3;
				return TRUE;

				case IDC_CHECK1:
				{
					if (IsDlgButtonChecked(dialog, IDC_CHECK1) == BST_CHECKED)
						searchAddFreeze = 1;
					else
						searchAddFreeze = 0;
				}
			}
		}
	}
	return FALSE;
}
//==============================================================================
BOOL CALLBACK CheatsListBox_Proc(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG: 
		{
			LV_COLUMN lvColumn;
				u8	proc = 0;
				u32 address = 0;
				u32 val = 0;

				cheatListView = GetDlgItem(dialog, IDC_LIST1);

				ListView_SetExtendedListViewStyle(cheatListView, LVS_EX_FULLROWSELECT);
				
				memset(&lvColumn,0,sizeof(LV_COLUMN));
				lvColumn.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
	
				lvColumn.fmt=LVCFMT_CENTER;
				lvColumn.cx=20;
				lvColumn.pszText="X";
				ListView_InsertColumn(cheatListView, 0, &lvColumn);

				lvColumn.fmt=LVCFMT_LEFT;
				lvColumn.cx=84;
				lvColumn.pszText="Address";
				ListView_InsertColumn(cheatListView, 1, &lvColumn);
				lvColumn.cx=100;
				lvColumn.pszText="Value";
				ListView_InsertColumn(cheatListView, 2, &lvColumn);
				lvColumn.cx=200;
				lvColumn.pszText="Description";
				ListView_InsertColumn(cheatListView, 3, &lvColumn);
				lvColumn.fmt=LVCFMT_CENTER;
				lvColumn.cx=45;
				lvColumn.pszText="CPU";
				ListView_InsertColumn(cheatListView, 4, &lvColumn);

				LVITEM lvi;
				memset(&lvi,0,sizeof(LVITEM));
				lvi.mask = LVIF_TEXT|LVIF_STATE;
				lvi.iItem = INT_MAX;

				
				cheatsGetListReset();
				SendMessage(cheatListView, WM_SETREDRAW, (WPARAM)FALSE,0);
				while (cheatsGetList(&tempCheat))
				{
					char buf[256];
					if (tempCheat.enabled)
						lvi.pszText= "X";
					else
						lvi.pszText= "";
					u32 row = ListView_InsertItem(cheatListView, &lvi);
					wsprintf(buf, "0x02%06X", tempCheat.hi[0]);
					ListView_SetItemText(cheatListView, row, 1, buf);
					ltoa(tempCheat.lo[0], buf, 10);
					ListView_SetItemText(cheatListView, row, 2, buf);
					ListView_SetItemText(cheatListView, row, 3, tempCheat.description);
					ListView_SetItemText(cheatListView, row, 4, NAME_CPUs[tempCheat.proc]);
				}
				SendMessage(cheatListView, WM_SETREDRAW, (WPARAM)TRUE,0);

				ListView_SetItemState(searchListView,0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
				SetFocus(searchListView);
			return TRUE;
		}
	
		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(dialog, FALSE);
				return TRUE;

				case IDC_BADD:
				{
					searchAddProc = 0;
					searchAddAddress = 0;;
					searchAddValue = 0;
					searchAddMode = 0;
					searchAddFreeze = 1;
					if (DialogBox(hAppInst, MAKEINTRESOURCE(IDD_CHEAT_ADD), dialog, (DLGPROC) CheatsAddProc))
					{
						LVITEM lvi;
						char buf[256];

						memset(&lvi,0,sizeof(LVITEM));
						lvi.mask = LVIF_TEXT|LVIF_STATE;
						lvi.iItem = INT_MAX;

						if (searchAddFreeze)
							lvi.pszText= "X";
						else
							lvi.pszText= " ";
						u32 row = ListView_InsertItem(cheatListView, &lvi);
						wsprintf(buf, "0x02%06X", searchAddAddress);
						ListView_SetItemText(cheatListView, row, 1, buf);
						ltoa(searchAddValue, buf, 10);
						ListView_SetItemText(cheatListView, row, 2, buf);
						ListView_SetItemText(cheatListView, row, 3, editBuf[2]);
						ListView_SetItemText(cheatListView, row, 4, NAME_CPUs[searchAddProc]);

						EnableWindow(GetDlgItem(dialog, IDOK), TRUE);
					}
				}
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

void CheatsListDialog(HWND hwnd)
{
	if (!cheatsPush()) return;
	memset(&tempCheat, 0, sizeof(CHEATS_LIST));
	u32 res=DialogBox(hAppInst, MAKEINTRESOURCE(IDD_CHEAT_LIST), hwnd, (DLGPROC) CheatsListBox_Proc);
	if (res)
	{
		cheatsSave();
		cheatsStackClear();
	}
	else
	{
		cheatsPop();
	}
}


// ================================================================================== Search
BOOL CALLBACK CheatsSearchExactWnd(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG: 
		{
			EnableWindow(hBRestart, TRUE);
			if (searchNumberResults)
				EnableWindow(hBView, TRUE);
			else
				EnableWindow(hBView, FALSE);
			EnableWindow(hBSearch, FALSE);
			
			SendMessage(GetDlgItem(dialog, IDC_EVALUE), EM_SETLIMITTEXT, 10, 0);
			SetWindowText(GetDlgItem(dialog, IDC_STATIC_RANGE), searchRangeText[searchSign][searchSize]);
			oldEditProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(dialog, IDC_EVALUE), GWLP_WNDPROC, (LONG)EditValueProc);
			char buf[256];
			ltoa(searchNumberResults, buf, 10);
			SetWindowText(GetDlgItem(dialog, IDC_SNUMBER), buf);
			SetFocus(GetDlgItem(dialog, IDC_EVALUE));
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDC_EVALUE:
				{
					if (HIWORD(wparam) == EN_UPDATE)
					{
						char buf[10];
						GetWindowText(GetDlgItem(dialog, IDC_EVALUE), buf, 10);
						if (!strlen(buf))
						{
							EnableWindow(hBSearch, FALSE);
							return TRUE;
						}
						
						u32 val = atol(buf);
						if (val > searchRange[searchSize][1])
						{
							EnableWindow(hBSearch, FALSE);
							return TRUE;
						}
						EnableWindow(hBSearch, TRUE);
						exactVal = val;
					}
					return TRUE;
				}
			}
			break;
		}
	}
	return FALSE;
}

BOOL CALLBACK CheatsSearchCompWnd(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG: 
		{
			EnableWindow(hBRestart, TRUE);
			if (searchNumberResults)
				EnableWindow(hBView, TRUE);
			else
				EnableWindow(hBView, FALSE);
			EnableWindow(hBSearch, TRUE);

			CheckDlgButton(dialog, searchCompIDDs[searchComp], BST_CHECKED);
			
			char buf[256];
			ltoa(searchNumberResults, buf, 10);
			SetWindowText(GetDlgItem(dialog, IDC_SNUMBER), buf);
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDC_RADIO1: searchComp = 0; break;
				case IDC_RADIO2: searchComp = 1; break;
				case IDC_RADIO3: searchComp = 2; break;
				case IDC_RADIO4: searchComp = 3; break;
			}
			break;
		}
	}
	return FALSE;
}

BOOL CALLBACK CheatsSearchResultWnd(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG: 
		{
			EnableWindow(hBRestart, TRUE);
			if (searchNumberResults)
				EnableWindow(hBView, TRUE);
			else
				EnableWindow(hBView, FALSE);
			EnableWindow(hBSearch, FALSE);
			char buf[256];
			ltoa(searchNumberResults, buf, 10);
			SetWindowText(GetDlgItem(dialog, IDC_SNUMBER), buf);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CALLBACK CheatsSearchViewWnd(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			{
				LV_COLUMN lvColumn;
				u8	proc = 0;
				u32 address = 0;
				u32 val = 0;

				searchListView = GetDlgItem(dialog, IDC_LIST);

				ListView_SetExtendedListViewStyle(searchListView, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
				
				memset(&lvColumn,0,sizeof(LV_COLUMN));
				lvColumn.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
				lvColumn.fmt=LVCFMT_CENTER;
				lvColumn.cx=40;
				lvColumn.pszText="CPU";
				ListView_InsertColumn(searchListView, 0, &lvColumn);
				lvColumn.fmt=LVCFMT_LEFT;
				lvColumn.cx=84;
				lvColumn.pszText="Address";
				ListView_InsertColumn(searchListView, 1, &lvColumn);
				lvColumn.cx=100;
				lvColumn.pszText="Value";
				ListView_InsertColumn(searchListView, 2, &lvColumn);

				LVITEM lvi;
				memset(&lvi,0,sizeof(LVITEM));
				lvi.mask = LVIF_TEXT|LVIF_STATE;
				lvi.iItem = INT_MAX;

				cheatSearchGetListReset();
				SendMessage(searchListView, WM_SETREDRAW, (WPARAM)FALSE,0);
				while (cheatSearchGetList(&proc, &address, &val))
				{
					char buf[256];
					char buf2[256];
					wsprintf(buf, "0x02%06X", address);
					_ltoa(val, buf2, 10);
					lvi.pszText= NAME_CPUs[proc];
					u32 row = SendMessage(searchListView, LVM_INSERTITEM, 0, (LPARAM)&lvi);
					ListView_SetItemText(searchListView, row, 1, buf);
					ListView_SetItemText(searchListView, row, 2, buf2);
				}
				SendMessage(searchListView, WM_SETREDRAW, (WPARAM)TRUE,0);
				ListView_SetItemState(searchListView,0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
				SetFocus(searchListView);
			}
		return TRUE;
		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDCANCEL:
					ListView_DeleteAllItems(searchListView);
					EndDialog(dialog, FALSE);
				return TRUE;
				case IDC_BADD:
					{
						u32 val = 0;
						char buf[12];
						u32 pos = ListView_GetNextItem(searchListView, -1, LVNI_SELECTED|LVNI_FOCUSED);
						ListView_GetItemText(searchListView, pos, 0, buf, 4);
						searchAddProc = (buf[3] == '7');
						ListView_GetItemText(searchListView, pos, 1, buf, 12);
						sscanf_s(buf, "%x", &val);
						searchAddAddress = val & 0x00FFFFFF;
						ListView_GetItemText(searchListView, pos, 2, buf, 12);
						searchAddValue = atol(buf);
						searchAddMode = 1;
						searchAddSize = searchSize;
						DialogBox(hAppInst, MAKEINTRESOURCE(IDD_CHEAT_ADD), dialog, (DLGPROC) CheatsAddProc);
					}
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL CALLBACK CheatsSearchMainWnd(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG: 
		{
			CheckDlgButton(dialog, searchSizeIDDs[searchSize], BST_CHECKED);
			CheckDlgButton(dialog, searchSignIDDs[searchSign], BST_CHECKED);
			CheckDlgButton(dialog, searchTypeIDDs[searchType], BST_CHECKED);
			for (int i = 0; i < 4; i++)
				SetWindowText(GetDlgItem(dialog, searchRangeIDDs[i]), searchRangeText[searchSign][i]);
			EnableWindow(hBRestart, FALSE);
			EnableWindow(hBView, FALSE);
			EnableWindow(hBSearch, TRUE);
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDC_RADIO1:		// 1 byte
					searchSize = 0;
				return TRUE;
				case IDC_RADIO2:		// 2 bytes
					searchSize = 1;
				return TRUE;
				case IDC_RADIO3:		// 3 bytes
					searchSize = 2;
				return TRUE;
				case IDC_RADIO4:		// 4 bytes
					searchSize = 3;
				return TRUE;

				case IDC_RADIO5:		// unsigned
				{
					searchSign = 0;
					for (int i = 0; i < 4; i++)
						SetWindowText(GetDlgItem(dialog, searchRangeIDDs[i]), searchRangeText[searchSign][i]);
					return TRUE;
				}
				case IDC_RADIO6:		//signed
				{
					searchSign = 1;
					for (int i = 0; i < 4; i++)
						SetWindowText(GetDlgItem(dialog, searchRangeIDDs[i]), searchRangeText[searchSign][i]);
					return TRUE;
				}

				case IDC_RADIO7:		// exact value search
					searchType = 0;
				return TRUE;
				case IDC_RADIO8:		// comparative search
					searchType = 1;
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

DLGPROC CheatsSearchSubWnds[2][4] = {
	{ CheatsSearchMainWnd, CheatsSearchExactWnd, CheatsSearchResultWnd, NULL },
	{ CheatsSearchMainWnd, CheatsSearchResultWnd, CheatsSearchCompWnd, CheatsSearchResultWnd }
};

//==============================================================================
BOOL CALLBACK CheatsSearchProc(HWND dialog, UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG: 
		{
			hBRestart = GetDlgItem(dialog, IDC_BRESTART);
			hBView =	GetDlgItem(dialog, IDC_BVIEW);
			hBSearch =	GetDlgItem(dialog, IDC_BSEARCH);
			
			searchWnd=CreateDialog(hAppInst, MAKEINTRESOURCE(searchIDDs[searchType][searchStep]), 
										dialog, (DLGPROC)CheatsSearchSubWnds[searchType][searchStep]);
			return TRUE;
		}
	
		case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
				case IDOK:
				case IDCANCEL:
					if (searchWnd) DestroyWindow(searchWnd);
					EndDialog(dialog, FALSE);
				return TRUE;

				case IDC_BVIEW:
					DialogBox(hAppInst, MAKEINTRESOURCE(IDD_CHEAT_SEARCH_VIEW), dialog, (DLGPROC)CheatsSearchViewWnd);
				return TRUE;

				case IDC_BSEARCH:
					if (searchStep == 0)
						cheatsSearchInit(searchType, searchSize, searchSign);
					if (searchType == 0)
					{
						if (searchStep == 1)
							searchNumberResults = cheatsSearchValue(exactVal);
					}
					else
					{
						if (searchStep == 2)
							searchNumberResults = cheatsSearchComp(searchComp);
					}

					searchStep++;
					if (searchWnd) DestroyWindow(searchWnd);
					searchWnd=CreateDialog(hAppInst, MAKEINTRESOURCE(searchIDDs[searchType][searchStep]), 
										dialog, (DLGPROC)CheatsSearchSubWnds[searchType][searchStep]);
					if (searchType == 0)
					{
						if (searchStep == 2) searchStep = 1;
					}
					else
					{
						if (searchStep == 1) searchStep = 2;
						if (searchStep == 3) searchStep = 2;
					}
				return TRUE;

				case IDC_BRESTART:
					cheatsSearchClose();
					searchStep = 0;
					searchNumberResults = 0;
					if (searchWnd) DestroyWindow(searchWnd);
					searchWnd=CreateDialog(hAppInst, MAKEINTRESOURCE(searchIDDs[searchType][searchStep]), 
										dialog, (DLGPROC)CheatsSearchSubWnds[searchType][searchStep]);
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

void CheatsSearchDialog(HWND hwnd)
{
	DialogBox(hAppInst, MAKEINTRESOURCE(IDD_CHEAT_SEARCH), hwnd, (DLGPROC) CheatsSearchProc);
}

void CheatsSearchReset()
{
	searchType = 0;
	searchSize = 0;
	searchSign = 0;
	searchStep = 0;
	searchComp = 0;
	searchAddSize = 0;
	exactVal = 0;
	searchNumberResults = 0;
}
