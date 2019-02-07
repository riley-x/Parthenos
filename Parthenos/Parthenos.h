#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"
#include "D2Objects.h"
#include "TitleBar.h"
#include "FileIO.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	// Logical components
	TitleBar m_titleBar;
	MouseTrackEvents m_mouseTrack;
	FileIO m_histFile;

	// Resource management.
	D2Objects				m_d2;
	//D2D1_POINT_2F			ptMouseStart;
	HCURSOR					hCursor;


	LRESULT	OnCreate();
	LRESULT OnPaint();
	LRESULT OnMouseMove(POINT cursor, DWORD flags);
	LRESULT OnSize(WPARAM wParam);
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT	OnLButtonDown(POINT cursor, DWORD flags);
	//void	OnLButtonUp();
	//void	OnMouseMove(int pixelX, int pixelY, DWORD flags);

	//void    Resize();

public:

	Parthenos() : BorderlessWindow() {}
	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName) {}

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// Handle initializations here instead of create
	// ClientRect doesn't have correct size during WM_CREATE
	void PreShow(); 

};