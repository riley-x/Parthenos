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
	void Paint(D2D1_RECT_F updateRect);
	void SetSize(D2D1_RECT_F dipRect); // This creates the text layouts and wedge geometries
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	//bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	//void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	//void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	void ProcessCTPMessages();

	// Interface
	void Load(std::vector<double> const & data, std::vector<std::wstring> const & shortLabels,
		std::vector<std::wstring> const & longLabels); // Call before SetSize


private:

	// Parameters
	float m_pad = 30.0f;
	float m_trueRadius; // radius in DIPs
	float m_innerRadius = 0.7f; // fraction of true radius
	float m_outerRadius = 1.0f; // fraction of true radius

	// State
	int m_mouseOn = -1;

	// Data
	double m_total;
	std::vector<double>			m_data;
	std::vector<std::wstring>	m_shortLabels;
	std::vector<std::wstring>	m_longLabels; // Text at center

	// Painting
	std::vector<ID2D1PathGeometry*> m_wedges;
	std::vector<IDWriteTextLayout*> m_shortLabelLayouts;
	std::vector<IDWriteTextLayout*> m_longLabelLayouts;

	// Helpers
	void CreateWedge(size_t i, float degrees);
	float CalculateTrueRadius(D2D1_RECT_F dipRect);
};