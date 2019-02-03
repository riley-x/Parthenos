#pragma once
#include "stdafx.h"
#include "D2Objects.h"

class TitleBar
{
public:
	TitleBar()
	{
		m_cRect.left = 0;
		m_cRect.top = 0;
		m_cRect.right = 0;
		m_cRect.bottom = 0;
	}

	void Paint(D2Objects const & d2);
	void Resize(RECT pRect);
	void LoadCommandIcons(D2Objects & d2);

	int left() const { return m_cRect.left; }
	int top() const { return m_cRect.top; }
	int right() const { return m_cRect.right; }
	int bottom() const { return m_cRect.bottom; }
	int width() const { return m_cRect.right - m_cRect.left; }
	int height() const { return m_cRect.bottom - m_cRect.top; }

private:
	RECT		m_cRect; // pixels in main window client coordinates

	// Resources
	HRSRC					imageResHandle;
	HGLOBAL					imageResDataHandle;
	void					*pImageFile;
	DWORD					imageFileSize;

};