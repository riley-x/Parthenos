#pragma once

#include "stdafx.h"
#include "resource.h"
#include "BorderlessWindow.h"
#include "D2Objects.h"
#include "TitleBar.h"
#include "FileIO.h"
#include "Chart.h"


class Parthenos : public BorderlessWindow<Parthenos>
{
	// Logical components
	std::vector<AppItem*>	m_activeItems; // items that need to be painted
	std::vector<AppItem*>	m_allItems;
	TitleBar				*m_titleBar;
	Chart					*m_chart;
	MouseTrackEvents		m_mouseTrack;
	FileIO					m_histFile;

	// Resource management.
	D2Objects				m_d2;
	//D2D1_POINT_2F			ptMouseStart;
	HCURSOR					hCursor;

	// Data memebers
	float m_leftPanelWidth = 350.0f; // in DIPs

	LRESULT	OnCreate();
	LRESULT OnPaint();
	LRESULT OnMouseMove(POINT cursor, WPARAM wParam);
	LRESULT OnChar(wchar_t c, LPARAM lParam);
	bool	OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(WPARAM wParam);
	LRESULT OnNCHitTest(POINT cursor);
	LRESULT	OnLButtonDown(POINT cursor, WPARAM wParam);
	LRESULT OnLButtonDblclk(POINT cursor, WPARAM wParam);
	LRESULT	OnLButtonUp(POINT cursor, WPARAM wParam);

public:

	Parthenos() : BorderlessWindow() {}
	Parthenos(PCWSTR szClassName) : BorderlessWindow(szClassName) {}
	~Parthenos();

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// Handle initializations here instead of create
	// ClientRect doesn't have correct size during WM_CREATE
	void PreShow(); 

};