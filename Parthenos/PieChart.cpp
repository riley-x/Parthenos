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
	m_d2.pRenderTarget->FillRectangle(m_dipRect, m_d2.pBrush);

	// Wedges
	float offset_angle = 0.0f;
	for (size_t i = 0; i < m_angles.size(); i++)
	{
		PaintWedge(m_wedges[i], offset_angle, m_colors[i]);
		offset_angle += m_angles[i];
	}
	m_d2.pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

	// Borders ( this is cleaner than drawing the geometry borders
	m_d2.pBrush->SetColor(Colors::ALMOST_WHITE);
	offset_angle = 0.0f;
	for (size_t i = 0; i < m_angles.size(); i++)
	{
		float offset_radians = static_cast<float>(M_PI / 180.0 * offset_angle);
		float x = m_trueRadius * sin(offset_radians);
		float y = m_trueRadius * -cos(offset_radians);
		m_d2.pRenderTarget->DrawLine(
			D2D1::Point2F(m_center.x + m_innerRadius * x, m_center.y + m_innerRadius * y),
			D2D1::Point2F(m_center.x + m_outerRadius * x, m_center.y + m_outerRadius * y),
			m_d2.pBrush, 2.0f
		);
		offset_angle += m_angles[i];
	}
	m_d2.pRenderTarget->DrawEllipse(
		D2D1::Ellipse(m_center, m_trueRadius * m_innerRadius, m_trueRadius * m_innerRadius), 
		m_d2.pBrush, 2.0f
	);
	m_d2.pRenderTarget->DrawEllipse(
		D2D1::Ellipse(m_center, m_trueRadius * m_outerRadius, m_trueRadius * m_outerRadius),
		m_d2.pBrush, 2.0f
	);

	// Short labels
	m_d2.pBrush->SetColor(Colors::MAIN_TEXT);
	offset_angle = 0.0f;
	for (size_t i = 0; i < m_angles.size(); i++)
	{
		if (m_angles[i] >= 5.0f)
		{
			float center_angle = static_cast<float>(M_PI / 180.0 * (offset_angle + m_angles[i] / 2.0)); // radians
			float center_r = m_trueRadius * (m_innerRadius + m_outerRadius) / 2.0f;
			float center_x = center_r * sin(center_angle);
			float center_y = -center_r * cos(center_angle);

			m_d2.pRenderTarget->DrawTextLayout(
				D2D1::Point2F(m_center.x + center_x - m_shortLayoutSize / 2.0f, m_center.y + center_y - m_shortLayoutSize / 2.0f),
				m_shortLabelLayouts[i],
				m_d2.pBrush
			);
		}
		offset_angle += m_angles[i];
	}

	// Long label
	if (m_mouseOn >= 0)
	{
		// TODO
	}
	else if (!m_longLabelLayouts.empty())
	{
		m_d2.pRenderTarget->DrawTextLayout(
			D2D1::Point2F(m_center.x - m_longLayoutSize / 2.0f, m_center.y - m_longLayoutSize / 2.0f),
			m_longLabelLayouts[0],
			m_d2.pBrush
		);
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
	m_angles.resize(data.size());
	m_wedges.resize(data.size());

	double sum = std::accumulate(data.begin(), data.end(), 0.0);
	for (size_t i = 0; i < data.size(); i++)
	{
		m_angles[i] = static_cast<float>(360.0 * data[i] / sum);
		CreateWedge(&m_wedges[i], static_cast<float>(2.0 * M_PI * data[i] / sum));
	}
}


void PieChart::CreateWedge(ID2D1PathGeometry ** ppGeometry, float radians)
{
	ID2D1GeometrySink *pSink = NULL;

	HRESULT hr = m_d2.pFactory->CreatePathGeometry(ppGeometry);
	if (SUCCEEDED(hr))
	{
		hr = (*ppGeometry)->Open(&pSink);
	}
	if (SUCCEEDED(hr))
	{
		pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

		pSink->BeginFigure(
			D2D1::Point2F(0, -m_innerRadius), // Start on positive y-axis
			D2D1_FIGURE_BEGIN_FILLED
		);
		pSink->AddLine(
			D2D1::Point2F(0, -m_outerRadius)
		);
		pSink->AddArc(D2D1::ArcSegment(
			D2D1::Point2F(m_outerRadius * sin(radians), -m_outerRadius * cos(radians)), // end point
			D2D1::SizeF(m_outerRadius, m_outerRadius), // x,y radii
			0.0f, // rotation angle of ellipse axes
			D2D1_SWEEP_DIRECTION_CLOCKWISE,
			(radians > M_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
		));
		pSink->AddLine(
			D2D1::Point2F(m_innerRadius * sin(radians), -m_innerRadius * cos(radians))
		);
		pSink->AddArc(D2D1::ArcSegment(
			D2D1::Point2F(0, -m_innerRadius), // end point
			D2D1::SizeF(m_innerRadius, m_innerRadius), // x,y radii
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
	m_d2.pTextFormats[D2Objects::Formats::Segoe18]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
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
	m_d2.pRenderTarget->SetTransform(scale * rotation * translation);

	m_d2.pBrush->SetColor(color);
	m_d2.pRenderTarget->FillGeometry(pWedge, m_d2.pBrush);
	//m_d2.pBrush->SetColor(m_borderColor);
	//m_d2.pRenderTarget->DrawGeometry(pWedge, m_d2.pBrush, 2.0f, m_d2.pFixedTransformStyle);
}
