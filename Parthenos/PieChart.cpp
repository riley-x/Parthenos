#include "stdafx.h"
#include "PieChart.h"

PieChart::~PieChart()
{
	for (auto x : m_wedges) SafeRelease(&x);
	for (auto x : m_shortLabelLayouts) SafeRelease(&x);
	for (auto x : m_longLabelLayouts) SafeRelease(&x);
}

void PieChart::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(m_dipRect, dipRect)) return;

	float r = CalculateTrueRadius(dipRect);
	if (r == m_trueRadius) return;

	////
}
