#include "stdafx.h"
#include "PieChart.h"

PieChart::~PieChart()
{
	for (auto x : m_wedges) SafeRelease(&x);
	for (auto x : m_sliders) SafeRelease(&x);
	for (auto x : m_shortLabelLayouts) SafeRelease(&x);
	for (auto x : m_longLabelLayouts) SafeRelease(&x);
}

void PieChart::SetSize(D2D1_RECT_F dipRect)
{
	if (equalRect(m_dipRect, dipRect)) return;
	if (dipRect.right - dipRect.left < 200.0f ||
		dipRect.bottom - dipRect.top < 200.0f) return;

	m_dipRect = dipRect;
	m_pixRect = DPIScale::DipsToPixels(dipRect);
	
	m_center = D2D1::Point2F((m_dipRect.left + m_dipRect.right) / 2.0f, (m_dipRect.top + m_dipRect.bottom) / 2.0f);
	m_trueRadius = CalculateTrueRadius(dipRect);
	m_shortLayoutSize = m_trueRadius * (m_outerRadius - m_innerRadius); // Approximate bounding box as square r2 - r1
	m_longLayoutSize = m_trueRadius * m_innerRadius * 1.414f; // Bounding box is square r1 * sqrt(2)

	CreateTextLayouts();
}

void PieChart::Paint(D2D1_RECT_F updateRect)
{
	if (!overlapRect(m_dipRect, updateRect)) return;

	// Background
	m_d2.pBrush->SetColor(Colors::AXES_BACKGROUND);
	m_d2.pD2DContext->FillRectangle(m_dipRect, m_d2.pBrush);

	// Wedges
	for (size_t i = 0; i < m_wedges.size(); i++)
		PaintWedge(m_wedges[i], m_angles[i], m_colors[i]);

	// Sliders
	for (size_t i = 0; i < m_sliders.size(); i++)
		PaintWedge(m_sliders[i], m_sliderPos[i].first, m_sliderColors[i]);
	m_d2.pD2DContext->SetTransform(D2D1::Matrix3x2F::Identity());


	// Borders (this is cleaner than drawing the geometry borders)
	m_d2.pBrush->SetColor(m_borderColor);
	for (size_t i = 0; i < m_angles.size(); i++)
	{
		float offset_radians = static_cast<float>(M_PI / 180.0 * m_angles[i]);
		float x = m_trueRadius * sin(offset_radians);
		float y = m_trueRadius * -cos(offset_radians);
		m_d2.pD2DContext->DrawLine(
			D2D1::Point2F(m_center.x + m_innerRadius * x, m_center.y + m_innerRadius * y),
			D2D1::Point2F(m_center.x + m_outerRadius * x, m_center.y + m_outerRadius * y),
			m_d2.pBrush, 2.0f
		);
	}
	m_d2.pD2DContext->DrawEllipse(
		D2D1::Ellipse(m_center, m_trueRadius * m_innerRadius, m_trueRadius * m_innerRadius), 
		m_d2.pBrush, 2.0f
	);
	m_d2.pD2DContext->DrawEllipse(
		D2D1::Ellipse(m_center, m_trueRadius * m_outerRadius, m_trueRadius * m_outerRadius),
		m_d2.pBrush, 2.0f
	);

	// Short labels
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	for (size_t i = 0; i < m_angles.size(); i++)
	{
		if (m_widths[i] >= 5.0f)
		{
			float center_angle = static_cast<float>(M_PI / 180.0 * (m_angles[i] + m_widths[i] / 2.0)); // radians
			float center_r = m_trueRadius * (m_innerRadius + m_outerRadius) / 2.0f;
			float center_x = center_r * sin(center_angle);
			float center_y = -center_r * cos(center_angle);

			m_d2.pD2DContext->DrawTextLayout(
				D2D1::Point2F(m_center.x + center_x - m_shortLayoutSize / 2.0f, m_center.y + center_y - m_shortLayoutSize / 2.0f),
				m_shortLabelLayouts[i],
				m_d2.pBrush
			);
		}
	}

	// Long label
	m_d2.pD2DContext->DrawTextLayout(
		D2D1::Point2F(m_center.x - m_longLayoutSize / 2.0f, m_center.y - m_longLayoutSize / 2.0f),
		m_longLabelLayouts[m_mouseOn + 1],
		m_d2.pBrush
	);
}

bool PieChart::OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled)
{
	D2D1_POINT_2F point = D2D1::Point2F(cursor.x - m_center.x, cursor.y - m_center.y);
	if (inRect(cursor, m_dipRect) && !handeled && inRadius(point, m_trueRadius * m_outerRadius)
		&& !inRadius(point, m_trueRadius * m_sliderRadius))
	{
		float degree = std::atan2(point.x, -point.y) * 180.0f / (float)M_PI;
		if (degree < 0) degree += 360.0f;

		if (inRadius(point, m_trueRadius * m_innerRadius)) // slider
		{
			for (size_t i = 0; i < m_sliderPos.size(); i++)
			{
				if (degree >= m_sliderPos[i].first && degree <= m_sliderPos[i].second)
				{
					if (m_mouseOn != m_wedges.size() + i)
					{
						m_mouseOn = m_wedges.size() + i;
						::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
					}
					return true;
				}
			}
			if (m_mouseOn >= 0) // not hovering over a slider
			{
				m_mouseOn = -1;
				::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
			}
			return false;
		}
		else // main wedges
		{
			for (size_t i = 0; i < m_widths.size(); i++)
			{
				degree -= m_widths[i];
				if (degree < 0)
				{
					if (m_mouseOn != i)
					{
						m_mouseOn = i;
						::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
					}
					break;
				}
			}
			return true;
		}
	}
	else // not in pie
	{
		if (m_mouseOn >= 0)
		{
			m_mouseOn = -1;
			::InvalidateRect(m_hwnd, &m_pixRect, FALSE);
		}
		return false;
	}
}

void PieChart::Load(std::vector<double> const & data, std::vector<D2D1_COLOR_F> const & colors,
	std::vector<std::wstring> const & shortLabels, std::vector<std::wstring> const & longLabels)
{
	if (shortLabels.size() != data.size() || longLabels.size() != data.size() + 1
		|| colors.size() != data.size())
		throw std::invalid_argument("Inconsistent labels for pie chart");
	
	m_shortLabels = shortLabels;
	m_longLabels = longLabels;
	m_colors = colors;

	// Create the wedges
	for (auto x : m_wedges) SafeRelease(&x);
	m_widths.resize(data.size());
	m_angles.resize(data.size());
	m_wedges.resize(data.size());

	m_sum = std::accumulate(data.begin(), data.end(), 0.0);
	for (size_t i = 0; i < data.size(); i++)
	{
		if (i == 0) m_angles[i] = 0;
		else m_angles[i] = m_angles[i - 1] + m_widths[i - 1];
		m_widths[i] = static_cast<float>(360.0 * data[i] / m_sum);
		CreateWedge(&m_wedges[i], static_cast<float>(2.0 * M_PI * data[i] / m_sum));
	}
}

void PieChart::LoadSliders(std::vector<size_t> const & positions, std::vector<double> const & sizes, 
	std::vector<D2D1_COLOR_F> const & colors, std::vector<std::wstring> const & labels)
{
	if (sizes.size() != positions.size() || labels.size() != positions.size()
		|| colors.size() != positions.size())
		throw std::invalid_argument("PieChart::LoadSliders inconsistent argument lengths");

	m_sliderColors = colors;
	m_longLabels.insert(m_longLabels.end(), labels.begin(), labels.end());

	m_sliderPos.resize(positions.size());
	for (auto x : m_sliders) SafeRelease(&x);
	m_sliders.resize(positions.size());

	size_t curr_ind = -1;
	float curr_pos = 0;
	for (size_t i = 0; i < positions.size(); i++)
	{
		if (positions[i] >= m_angles.size()) throw ws_exception(L"PieChart::LoadSliders invalid positions");

		if (curr_ind != positions[i])
		{
			curr_ind = positions[i];
			curr_pos = m_angles[positions[i]];
		}
		float end_pos = curr_pos + static_cast<float>(360.0 * sizes[i] / m_sum); 
		m_sliderPos[i] = { curr_pos, end_pos }; 
		curr_pos = end_pos;

		CreateWedge(&m_sliders[i], static_cast<float>(2.0 * M_PI * sizes[i] / m_sum - .01), true); // -.01 == small border
	}
}

void PieChart::CreateWedge(ID2D1PathGeometry ** ppGeometry, float radians, bool slider)
{
	ID2D1GeometrySink *pSink = NULL;
	float r1 = (slider) ? m_sliderRadius : m_innerRadius; // smaller
	float r2 = (slider) ? m_innerRadius : m_outerRadius;  // larger

	HRESULT hr = m_d2.pFactory->CreatePathGeometry(ppGeometry);
	if (SUCCEEDED(hr))
	{
		hr = (*ppGeometry)->Open(&pSink);
	}
	if (SUCCEEDED(hr))
	{
		pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
		
		// Start on positive y-axis
		pSink->BeginFigure(
			D2D1::Point2F(0, -r1), 
			D2D1_FIGURE_BEGIN_FILLED
		);
		// Edge up
		pSink->AddLine(
			D2D1::Point2F(0, -r2)
		);
		// Outer arc clockwise
		pSink->AddArc(D2D1::ArcSegment(
			D2D1::Point2F(r2 * sin(radians), -r2 * cos(radians)), // end point
			D2D1::SizeF(r2, r2), // x,y radii
			0.0f, // rotation angle of ellipse axes
			D2D1_SWEEP_DIRECTION_CLOCKWISE,
			(radians > M_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
		));
		// Edge in
		pSink->AddLine(
			D2D1::Point2F(r1 * sin(radians), -r1 * cos(radians))
		);
		// Inner arc counter-clockwise
		pSink->AddArc(D2D1::ArcSegment(
			D2D1::Point2F(0, -r1), // end point
			D2D1::SizeF(r1, r1), // x,y radii
			0.0f, // rotation angle of ellipse axes
			D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
			(radians > M_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
		));
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
		hr = pSink->Close();
		SafeRelease(&pSink);
	}
	if (FAILED(hr))
	{
		throw Error(L"CreateWedge failed");
	}
}

// Creates the layouts for the labels.
void PieChart::CreateTextLayouts()
{
	// Short labels
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (auto x : m_shortLabelLayouts) SafeRelease(&x);
	m_shortLabelLayouts.resize(m_shortLabels.size());
	for (size_t i = 0; i < m_shortLabels.size(); i++)
	{
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_shortLabels[i].c_str(),						// The string to be laid out and formatted.
			m_shortLabels[i].size(),						// The length of the string.
			m_d2.pTextFormats[D2Objects::Formats::Segoe12],	// The text format to apply to the string
			m_shortLayoutSize,								// The width of the layout box.
			m_shortLayoutSize,								// The height of the layout box.
			&m_shortLabelLayouts[i]							// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"PieChart: CreateTextLayout failed");
	}
	m_d2.pTextFormats[D2Objects::Formats::Segoe12]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

	// Long labels
	m_d2.pTextFormats[D2Objects::Formats::Segoe18]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (auto x : m_longLabelLayouts) SafeRelease(&x);
	m_longLabelLayouts.resize(m_longLabels.size());
	for (size_t i = 0; i < m_longLabels.size(); i++)
	{
		HRESULT hr = m_d2.pDWriteFactory->CreateTextLayout(
			m_longLabels[i].c_str(),						// The string to be laid out and formatted.
			m_longLabels[i].size(),							// The length of the string.
			m_d2.pTextFormats[D2Objects::Formats::Segoe18],	// The text format to apply to the string
			m_longLayoutSize,								// The width of the layout box.
			m_longLayoutSize,								// The height of the layout box.
			&m_longLabelLayouts[i]							// The IDWriteTextLayout interface pointer.
		);
		if (FAILED(hr)) throw Error(L"PieChart: CreateTextLayout failed");
	}
}

float PieChart::CalculateTrueRadius(D2D1_RECT_F dipRect)
{
	float width = m_dipRect.right - m_dipRect.left - m_pad;
	float height = m_dipRect.bottom - m_dipRect.top - m_pad;
	return min(width, height) / 2.0f;
}

// Does not reset the render target transform
void PieChart::PaintWedge(ID2D1PathGeometry * pWedge, float offset_angle, D2D1_COLOR_F color)
{
	D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
		m_trueRadius,
		m_trueRadius,
		D2D1::Point2F(0, 0)
	);
	D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
		offset_angle,
		D2D1::Point2F(0, 0)
	);
	D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(m_center.x, m_center.y);
	m_d2.pD2DContext->SetTransform(scale * rotation * translation);

	m_d2.pBrush->SetColor(color);
	m_d2.pD2DContext->FillGeometry(pWedge, m_d2.pBrush);
	//m_d2.pBrush->SetColor(m_borderColor);
	//m_d2.pD2DContext->DrawGeometry(pWedge, m_d2.pBrush, 2.0f, m_d2.pFixedTransformStyle);
}
