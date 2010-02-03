// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#ifndef __CONFIGDLG_H__
#define __CONFIGDLG_H__

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/gbsizer.h>

#if defined(HAVE_X11) && HAVE_X11
#include "InputCommon/X11InputBase.h"
#endif

class PADConfigDialogSimple : public wxDialog
{

		
	public:
		PADConfigDialogSimple(wxWindow *parent, wxWindowID id = 1, const wxString &title = wxT("Pad Configuration"),
			const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE);
		
		virtual ~PADConfigDialogSimple();

	private:
		DECLARE_EVENT_TABLE();
		wxNotebook *m_Notebook;
		wxPanel *m_Controller[4];
		wxButton *m_About;
		wxButton *m_Close;

		wxStaticBoxSizer *sbDevice[4], *m_SizeXInput[4], *m_SizeRecording[4];
		wxBoxSizer *sDevice[4];
		wxGridBagSizer *sPage[4];
		wxStaticBoxSizer *sButtons[4];
		wxStaticBoxSizer *sTriggers[4];
		wxStaticBoxSizer *sStick[4];
		wxStaticBoxSizer *sCStick[4];
		wxStaticBoxSizer *sDPad[4];

		wxArrayString arrayStringFor_X360Pad;
		wxCheckBox *m_X360Pad[4];
		wxChoice *m_X360PadC[4];
		wxCheckBox *m_Disable[4];
		wxCheckBox *m_Rumble[4];

		// Recording
		wxCheckBox *m_CheckRecording[4];
		wxCheckBox *m_CheckPlayback[4];
		wxButton *m_BtnSaveRecording[4];

		wxButton *m_ButtonA[4];
		wxButton *m_ButtonB[4];
		wxButton *m_ButtonX[4];
		wxButton *m_ButtonY[4];
		wxButton *m_ButtonZ[4];
		wxButton *m_ButtonStart[4];
		wxButton *m_ButtonL[4];
		wxButton *m_ButtonR[4];
		wxButton *m_ButtonL_Semi[4];
		wxButton *m_ButtonR_Semi[4];
		wxSlider *m_Trigger_SemiValue[4];
		wxStaticText *m_Trigger_SemiValue_Label[4];
		wxButton *m_StickUp[4];
		wxButton *m_StickDown[4];
		wxButton *m_StickLeft[4];
		wxButton *m_StickRight[4];
		wxButton *m_Stick_Semi[4];
		wxSlider *m_Stick_SemiValue[4];
		wxStaticText *m_Stick_SemiValue_Label[4];
		wxButton *m_CStickUp[4];
		wxButton *m_CStickDown[4];
		wxButton *m_CStickLeft[4];
		wxButton *m_CStickRight[4];
		wxButton *m_CStick_Semi[4];
		wxSlider *m_CStick_SemiValue[4];
		wxStaticText *m_CStick_SemiValue_Label[4];
		wxButton *m_DPadUp[4];
		wxButton *m_DPadDown[4];
		wxButton *m_DPadLeft[4];
		wxButton *m_DPadRight[4];
		
		enum
		{
			////GUI Enum Control ID Start
			ID_CLOSE = 1000,
			ID_NOTEBOOK,
			ID_CONTROLLERPAGE1,
			ID_CONTROLLERPAGE2,
			ID_CONTROLLERPAGE3,
			ID_CONTROLLERPAGE4,

			// XInput pad
			ID_X360PAD_CHOICE,
			ID_X360PAD,
			ID_RUMBLE,

			// Semi-press values
			ID_TRIGGER_SEMIVALUE,
			ID_MAIN_SEMIVALUE,
			ID_SUB_SEMIVALUE,

			// Input recording
			ID_RECORDING,
			ID_PLAYBACK,
			ID_SAVE_RECORDING,

			// General settings
			ID_DISABLE,
			ID_PAD_ABOUT,
		};
	
		void OnClose(wxCloseEvent& event);
		void CreateGUIControls();
		void OnCloseClick(wxCommandEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		void ControllerSettingsChanged(wxCommandEvent& event);
		void OnButtonClick(wxCommandEvent& event);
		void DllAbout(wxCommandEvent& event);
		void OnShow(wxShowEvent& event);
		void AddSlider(wxPanel *pan, wxSlider **slider,
			wxStaticBoxSizer *sizer, const char *name, int ctl, int controller);

		wxButton *ClickedButton;
		wxString oldLabel;
};

#endif
