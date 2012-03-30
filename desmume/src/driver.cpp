/*
	Copyright (C) 2009-2010 DeSmuME team

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

#include "types.h"
#include "driver.h"
#include "rasterize.h"
#include "gfx3d.h"
#include "texcache.h"


#ifdef HAVE_WX
#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wxdlg/wxdlg3dViewer.h"

const int kViewportWidth = 256;
const int kViewportHeight = 192;

static SoftRasterizerEngine engine;
static Fragment _screen[kViewportWidth*kViewportHeight];
static FragmentColor _screenColor[kViewportWidth*kViewportHeight];

extern void _HACK_Viewer_ExecUnit(SoftRasterizerEngine* engine);

class Mywxdlg3dViewer : public wxdlg3dViewer
{
public:
	Mywxdlg3dViewer()
		: wxdlg3dViewer(NULL)
	{}

	virtual void RepaintPanel()
	{
		Refresh(false);
		Update();
	}

	void NewFrame()
	{
		listPolys->SetItemCount(viewer3d_state->polylist.count);
		labelFrameCounter->SetLabel(wxString::Format(wxT("Frame: %d"),viewer3d_state->frameNumber));
		labelUserPolycount->SetLabel(wxString::Format(wxT("User Polys: %d"),viewer3d_state->polylist.count));
		labelFinalPolycount->SetLabel(wxString::Format(wxT("Final Polys: %d"),viewer3d_state->polylist.count));
		//tree->DeleteAllItems();
		//tree->Freeze();
		//wxTreeItemId root = tree->AddRoot("");
		//for(int i=0;i<viewer3d_state->polylist.count;i++)
		//{
		//	tree->AppendItem(root,"hai kirin");
		//}
		//tree->Thaw();
	}

	virtual wxString OnGetItemText(const wxListCtrl* list, long item, long column) const
	{
		return wxT("hi");
	}

	virtual void OnListPolysSelected( wxListEvent& event )
	{
		panelTexture->Refresh(false);
		engine._debug_drawClippedUserPoly = GetSelectedListviewItem(listPolys);
	}

	void RedrawPanel(wxClientDC* dc)
	{
		//------------
		//do the 3d work..
		engine.polylist = &viewer3d_state->polylist;
		engine.vertlist = &viewer3d_state->vertlist;
		engine.indexlist = &viewer3d_state->indexlist;
		engine.screen = _screen;
		engine.screenColor = _screenColor;
		engine.width = kViewportWidth;
		engine.height = kViewportHeight;

		engine.updateFogTable();
	
		engine.initFramebuffer(kViewportWidth,kViewportHeight,gfx3d.state.enableClearImage?true:false);
		engine.updateToonTable();
		engine.updateFloatColors();
		engine.performClipping(checkMaterialInterpolate->IsChecked());
		engine.performViewportTransforms<true>(kViewportWidth,kViewportHeight);
		engine.performBackfaceTests();
		engine.performCoordAdjustment(false);
		engine.setupTextures(false);

		_HACK_Viewer_ExecUnit(&engine);
		//------------

		//dc.SetBackground(*wxGREEN_BRUSH); dc.Clear();
		u8 framebuffer[kViewportWidth*kViewportHeight*3];
		for(int y=0,i=0;y<kViewportHeight;y++)
			for(int x=0;x<kViewportWidth;x++,i++) {
				framebuffer[i*3] = _screenColor[i].r<<2;
				framebuffer[i*3+1] = _screenColor[i].g<<2;
				framebuffer[i*3+2] = _screenColor[i].b<<2;
			}
		wxImage image(kViewportWidth,kViewportHeight,framebuffer,true);
		wxBitmap bitmap(image);
		dc->DrawBitmap(bitmap,0,0);
	}

	virtual void _OnPaintPanel( wxPaintEvent& event )
	{
		wxClientDC dc(wxDynamicCast(event.GetEventObject(), wxWindow));
		RedrawPanel(&dc);
	}

	int GetSelectedListviewItem(wxListCtrl* list)
	{
		 return list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}

	virtual void OnPaintPanelTexture( wxPaintEvent& event )
	{
		wxPaintDC dc(wxDynamicCast(event.GetEventObject(), wxWindow));
		dc.SetBackground(*wxBLACK_BRUSH); dc.Clear();

		int selection = GetSelectedListviewItem(listPolys);
		if(selection < 0) return;
		if(selection>=viewer3d_state->polylist.count) return;

		POLY& poly = viewer3d_state->polylist.list[selection];

		TexCacheItem* texkey = TexCache_SetTexture(TexFormat_32bpp,poly.texParam,poly.texPalette);
		const u32 w = texkey->sizeX;
		const u32 h = texkey->sizeY;
		u8* const bmpdata = new u8[w*h*4];
		for(u32 i=0;i<w*h;i++) {
				bmpdata[i*3] = texkey->decoded[i*4];
				bmpdata[i*3+1] = texkey->decoded[i*4+1];
				bmpdata[i*3+2] = texkey->decoded[i*4+2];
			}
		for(u32 i=0;i<w*h;i++)
			bmpdata[w*h*3+i] = texkey->decoded[i*4+3];

		
		wxImage image(w,h,false);
		image.InitAlpha();
		image.SetData(bmpdata,true);
		image.SetAlpha(bmpdata+w*h*3,true);
		wxBitmap bitmap(image);
		double xscale = (double)panelTexture->GetSize().x / w;
		double yscale = (double)panelTexture->GetSize().y / h;

		dc.SetUserScale(xscale,yscale);
		dc.DrawBitmap(bitmap,0,0);
		delete[] bmpdata;
	}
};

class VIEW3D_Driver_WX : public VIEW3D_Driver
{
public:
	VIEW3D_Driver_WX()
		: viewer(NULL)
	{}
	~VIEW3D_Driver_WX()
	{
		delete viewer;
	}

	virtual bool IsRunning() { return viewer != NULL; }

	virtual void Launch()
	{
		if(viewer) return;
		delete viewer;
		viewer = new Mywxdlg3dViewer();
		viewer->Show(true);
	}

	void Close()
	{
		delete viewer;
		viewer = NULL;
	}

	virtual void NewFrame()
	{
		if(!viewer) return;
		if(!viewer->IsShown()) {
			Close();
			return;
		}

		viewer->NewFrame();
		viewer->RepaintPanel();
	}

private:
	Mywxdlg3dViewer *viewer;
};

#endif

static VIEW3D_Driver nullView3d;
BaseDriver::BaseDriver()
: view3d(NULL)
{
	VIEW3D_Shutdown();
}

void BaseDriver::VIEW3D_Shutdown()
{
	if(view3d != &nullView3d) delete view3d;
	view3d = &nullView3d;
}

void BaseDriver::VIEW3D_Init()
{
	VIEW3D_Shutdown();
#ifdef HAVE_WX
	view3d = new VIEW3D_Driver_WX();
#endif
}

BaseDriver::~BaseDriver()
{
}

