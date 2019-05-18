#pragma once
#include "../stdafx.h"
#include "AppItem.h"


// Plots a hollow pie chart. On mouse move, displays detailed info in the center.
class PieChart : public AppItem
{
public:
	
	// Constructor/Destructor
	using AppItem::AppItem;
	~PieChart();

	// AppItem overrides
	void SetSize(D2D1_RECT_F dipRect); // This creates the text layouts and wedge geometries
	void Paint(D2D1_RECT_F updateRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);

	// Interface

	// Extra entry at beginning of long labels for default center text. Call before SetSize.
	void Load(std::vector<double> const & data, std::vector<D2D1_COLOR_F> const & colors,
		std::vector<std::wstring> const & shortLabels, std::vector<std::wstring> const & longLabels);
	// Call after Load(). Positions should index into data above, sorted. Labels are long labels for hover-text.
	void LoadSliders(std::vector<size_t> const & positions, std::vector<double> const & sizes, 
		std::vector<D2D1_COLOR_F> const & colors, std::vector<std::wstring> const & labels);
	inline void Refresh() { CreateTextLayouts(); }

private:

	// Parameters
	const float m_pad = 30.0f;
	const float m_sliderRadius = 0.65f; // fraction of true radius
	const float m_innerRadius = 0.7f; // fraction of true radius
	const float m_outerRadius = 1.0f; // fraction of true radius

	// State
	int m_mouseOn = -1; // Which wedge/slider mouse currently on. -1 for no hover. Index as m_wedges[m_mouseOn],
						// or m_sliders[m_mouseOn - m_wedges.size()]

	// Data
	double m_sum; // sum of wedge real values
	std::vector<std::wstring>	m_shortLabels;
	std::vector<std::wstring>	m_longLabels = { L"" }; // Text at center, with extra entry at beginning for no mouseOn
														// Contains slider labels too (at end, see m_mouseOn)
	std::vector<D2D1_COLOR_F>	m_colors;
	std::vector<D2D1_COLOR_F>	m_sliderColors;
	std::vector<float>			m_widths; // width of wedge i in degrees.
	std::vector<float>			m_angles; // starting angle of wedge i in degrees.
	std::vector<std::pair<float,float>>	m_sliderPos; // (start, end) positions of slider i in degrees.

	// Painting
	float m_trueRadius; // radius in DIPs
	float m_shortLayoutSize;
	float m_longLayoutSize;
	D2D1_COLOR_F	m_borderColor = Colors::ALMOST_WHITE;
	D2D1_POINT_2F	m_center;
	std::vector<ID2D1PathGeometry*> m_wedges; // These have center at (0,0), start at theta = 0, CW, and normalized size
	std::vector<ID2D1PathGeometry*> m_sliders; // These have center at (0,0), start at theta = 0, CW, and normalized size
	std::vector<IDWriteTextLayout*> m_shortLabelLayouts;
	std::vector<IDWriteTextLayout*> m_longLabelLayouts;

	// Helpers
	void CreateWedge(ID2D1PathGeometry **ppGeometry, float radians, bool slider = false);
	void CreateTextLayouts();
	float CalculateTrueRadius(D2D1_RECT_F dipRect);
	void PaintWedge(ID2D1PathGeometry *pWedge, float offset_angle, D2D1_COLOR_F color); // in degrees
};