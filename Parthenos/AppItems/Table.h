#pragma once

#include "../stdafx.h"
#include "AppItem.h"
#include "ScrollBar.h"

class TableRowItem;
enum class ColumnType { Double, String };


class Table : public AppItem
{
public:
	Table(HWND hwnd, D2Objects const & d2, CTPMessageReceiver * parent, bool emptyEnd = true)
		: AppItem(hwnd, d2, parent), m_scrollBar(hwnd, d2, this), m_emptyEnd(emptyEnd)
	{
		// Set 1 line per minStep == detent
		m_scrollBar.SetMinStep(WHEEL_DELTA);
	}
	virtual ~Table();

	// AppItem
	void Paint(D2D1_RECT_F updateRect);
	void SetSize(D2D1_RECT_F dipRect);
	bool OnMouseMove(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnMouseWheel(D2D1_POINT_2F cursor, WPARAM wParam, bool handeled);
	bool OnLButtonDown(D2D1_POINT_2F cursor, bool handeled);
	void OnLButtonDblclk(D2D1_POINT_2F cursor, WPARAM wParam);
	void OnLButtonUp(D2D1_POINT_2F cursor, WPARAM wParam);
	bool OnChar(wchar_t c, LPARAM lParam);
	bool OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM wParam, LPARAM lParam);
	virtual void ProcessCTPMessages();
	bool ProcessTableCTP(ClientMessage const & msg);

	// Interface
	void SetColumns(std::vector<std::wstring> const & names, std::vector<float> const & widths, 
		std::vector<ColumnType> const & types);
	void Load(std::vector<TableRowItem*> const & items);
	inline void Refresh() { CalculatePositions(); CreateTextLayouts(); } // Load creates items but not with position, so call refresh
	
	inline std::vector<float> GetColumnWidths() const { return m_columnWidths; }
	inline std::vector<TableRowItem*> const & GetItems() const { return m_items; }

	// Statics
	static float const m_hTextPad; // 4.0f
	static D2Objects::Formats const m_format = D2Objects::Segoe12;

protected:
	// Objects
	std::vector<TableRowItem*>	m_items;

	// Data
	std::vector<std::wstring>	m_columnHeaders;
	std::vector<float>			m_columnWidths;
	std::vector<ColumnType>		m_columnTypes;

	// Flags
	bool m_ignoreSelection = false;	// Flag to check if drag + drop on same location

	// Helpers
	void CalculatePositions();
	void CreateTextLayouts();

	inline float GetHeight(int i) { return m_dipRect.top + m_headerHeight + (i - m_currLine) * m_rowHeight; }
		// Get top of item i
	inline int GetItem(float y) { return static_cast<int>(floor((y - (m_dipRect.top + m_headerHeight)) / m_rowHeight)) + static_cast<int>(m_currLine); }
		// Get index given y coord
	inline bool IsVisible(int i)
	{
		return i >= (int)m_currLine && i < (int)(m_currLine + m_visibleLines) && i < (int)m_items.size();
	}

	void MoveItem(int iOld, int iNew);
		// Move item iOld to iNew, shifting everything in between appropriately.
		// If iNew < iOld, moves iOld to above the current item at iNew.
		// If iNew > iOld, moves iOld to below the current item at iNew.
		// No bounds check. Make sure iOld and iNew are valid indices.
	void DeleteItem(size_t i);
	void SortByColumn(size_t iColumn);


private:
	// Objects
	ScrollBar						m_scrollBar;
	std::vector<IDWriteTextLayout*> m_columnLayouts; // For column headers

	// Flags
	bool		m_emptyEnd;					// Always maintain an 'empty' row at end
	int			m_LButtonDown = -1;			// Item index of drag start point
	int			m_hover = -1;				// Item index of where to draw hover line
	int			m_sortColumn = -1;			// So that double clicking on the same column sorts in reverse

	// Layout
	float const			m_headerHeight = 18.0f;
	float const			m_rowHeight = 18.0f;
	float				m_headerBorder;
	std::vector<float>	m_vLines;
	std::vector<float>	m_hLines;

	// Scroll State
	size_t		m_visibleLines;
	size_t		m_currLine = 0; // topmost visible line

	// Deleted
	Table(const Table&) = delete;				// non construction-copyable
	Table& operator=(const Table&) = delete;	// non copyable
};

class TableRowItem : public AppItem
{
public:
	TableRowItem(HWND hwnd, D2Objects const & d2, Table *parent) 
		: AppItem(hwnd, d2, parent), m_parent(parent) {}
	virtual ~TableRowItem() { for (auto item : m_items) if (item) delete item; }

	// Interface
	virtual double GetData(size_t iColumn) const { return 0.0; }
	virtual std::wstring GetString(size_t iColumn) const { return L""; }

protected:
	Table *m_parent;
	std::vector<AppItem*> m_items;

private:
	TableRowItem(const TableRowItem&) = delete; // non construction-copyable
	TableRowItem& operator=(const TableRowItem&) = delete; // non copyable
};