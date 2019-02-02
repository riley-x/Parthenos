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

private:
	RECT		m_cRect; // pixels in main window client coordinates

};