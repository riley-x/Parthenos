#pragma once
#include "stdafx.h"
#include "AppItem.h"


// Plots a hollow pie chart. On mouse move, displays detailed info in the center.
class PieChart : public AppItem
{
public:
	
	// Constructor/Destructor
	//PieChart(HWND hwnd, D2Objects const & d2);
	using AppItem::AppItem;
	~PieChart();

	// AppItem overrides
	void SetSize(D2D1_RECT_F dipRect); // This creates the text layouts and wedge geometries
	void Paint(D2D1_RECT_F updateRect);
	///bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	//bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	//void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	//void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	///void ProcessCTPMessages();

	// Interface

	// Extra entry at beginning of long labels for default center text
	// Call before SetSize
	void Load(std::vector<double> const & data, std::vector<D2D1_COLOR_F> const & colors,
		std::vector<std::wstring> const & shortLabels, std::vector<std::wstring> const & longLabels); 


private:

	// Parameters
	const float m_pad = 30.0f;
	const float m_innerRadius = 0.7f; // fraction of true radius
	const float m_outerRadius = 1.0f; // fraction of true radius

	// State
	int m_mouseOn = -1;

	// Data
	std::vector<std::wstring>	m_shortLabels;
	std::vector<std::wstring>	m_longLabels; // Text at center, with extra entry at beginning for no mouseOn
	std::vector<D2D1_COLOR_F>	m_colors;

	// Painting
	float m_trueRadius; // radius in DIPs
	float m_shortLayoutSize;
	float m_longLayoutSize;
	D2D1_COLOR_F					m_borderColor = Colors::BRIGHT_LINE;
	D2D1_POINT_2F					m_center;
	std::vector<float>				m_angles; // in degrees
	std::vector<ID2D1PathGeometry*> m_wedges; // These have center at (0,0), start at theta = 0, CC, and trueRadius 1
	std::vector<IDWriteTextLayout*> m_shortLabelLayouts;
	std::vector<IDWriteTextLayout*> m_longLabelLayouts;

	// Helpers
	void CreateWedge(ID2D1PathGeometry **ppGeometry, float radians);
	void CreateTextLayouts();
	float CalculateTrueRadius(D2D1_RECT_F dipRect);
	void PaintWedge(ID2D1PathGeometry *pWedge, float offset_angle, D2D1_COLOR_F color); // in degrees
};