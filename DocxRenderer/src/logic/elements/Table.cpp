#include "Table.h"

#include "../../resources/SingletonTemplate.h"
#include "../../resources/utils.h"

namespace NSDocxRenderer
{
	CTable::CCell::CCell(const CCell& other)
	{
		*this = other;
	}
	CTable::CCell::CCell(const double& left, const double& top, const double& right, const double& bot,
						 const CBorder& leftBorder, const CBorder& topBorder, const CBorder& rightBorder, const CBorder& botBorder)
	{
		m_dLeft		= left;
		m_dTop		= top;
		m_dRight	= right;
		m_dBot		= bot;
		m_dHeight	= bot - top;
		m_dWidth	= right - left;

		m_oLeftBorder	= leftBorder;
		m_oTopBorder	= topBorder;
		m_oRightBorder	= rightBorder;
		m_oBotBorder	= botBorder;
	}
	CTable::CCell::CCell(const double& left, const double& top, const double& right, const double& bot)
	{
		m_dLeft		= left;
		m_dTop		= top;
		m_dRight	= right;
		m_dBot		= bot;
		m_dHeight	= bot - top;
		m_dWidth	= right - left;
	}
	void CTable::CCell::Clear()
	{
		m_arParagraphs.clear();
	}
	void CTable::CCell::ToXml(NSStringUtils::CStringBuilder& oWriter) const
	{
		oWriter.WriteString(L"<w:tc>");
		oWriter.WriteString(L"<w:tcPr>");
		oWriter.WriteString(L"<w:tcW w:w=\"");
		oWriter.AddUInt(static_cast<unsigned int>(m_dWidth * c_dMMToDx));
		oWriter.WriteString(L"\" w:type=\"dxa\"/>");

		oWriter.WriteString(L"<w:gridSpan w:val=\"");
		oWriter.AddUInt(m_nGridSpan);
		oWriter.WriteString(L"\"/>");

		oWriter.WriteString(L"<w:vMerge w:val=\"");
		switch (m_eVMerge) {
		case eVMerge::vmContinue:
			oWriter.WriteString(L"continue");
			break;
		case eVMerge::vmRestart:
			oWriter.WriteString(L"restart");
			break;
		default:
			break;
		}
		oWriter.WriteString(L"\"/>");

		oWriter.WriteString(L"<w:tcBorders>");
		auto write_border = [&oWriter] (const CBorder& border, const std::wstring& prefix) {
			oWriter.WriteString(L"<w:");
			oWriter.WriteString(prefix);
			oWriter.WriteString(L" w:val=");
			oWriter.WriteString(SingletonInstance<LinesTable>().ConvertLineToString(border.lineType));
			oWriter.WriteString(L" w:sz=\"");
			oWriter.AddUInt(static_cast<unsigned int>(border.dWidth * c_dMMToPt * 4));
			oWriter.WriteString(L"\" w:space=\"");
			oWriter.AddUInt(static_cast<unsigned int>(border.dSpacing * c_dMMToPt));
			oWriter.WriteString(L"\" w:color=\"");
			if (border.lineType == eLineType::ltNone)
				oWriter.WriteString(L"auto");
			else
				oWriter.WriteHexInt3(ConvertColorBGRToRGB(border.lColor));
			oWriter.WriteString(L"\" />");
		};

		write_border(m_oTopBorder, L"top");
		write_border(m_oBotBorder, L"bottom");
		write_border(m_oLeftBorder, L"left");
		write_border(m_oRightBorder, L"right");

		oWriter.WriteString(L"</w:tcBorders>");

		if (m_eShading != eShading::shNone)
		{
			oWriter.WriteString(L"<w:shd w:val=");
			oWriter.WriteString(L"\"clear\"");
			oWriter.WriteString(L" w:fill=\"");
			oWriter.WriteHexInt3(ConvertColorBGRToRGB(m_lColor));
			oWriter.WriteString(L"\"/>");
		}

		oWriter.WriteString(L"</w:tcPr>");

		if (m_arParagraphs.empty())
			oWriter.WriteString(L"<w:p/>");

		for (const auto& p : m_arParagraphs)
			p->ToXml(oWriter);

		oWriter.WriteString(L"</w:tc>");
	}
	void CTable::CCell::ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const
	{

	}
	void CTable::CCell::ToBin(NSWasm::CData& oWriter) const
	{

	}
	CTable::CCell& CTable::CCell::operator=(const CCell& other)
	{
		CBaseItem::operator=(other);

		m_oBotBorder = other.m_oBotBorder;
		m_oTopBorder = other.m_oTopBorder;
		m_oLeftBorder = other.m_oLeftBorder;
		m_oRightBorder = other.m_oRightBorder;

		m_nGridSpan = other.m_nGridSpan;
		m_eVMerge = other.m_eVMerge;

		m_arParagraphs.clear();
		for (const auto& p : other.m_arParagraphs)
			m_arParagraphs.push_back(p);

		return *this;
	}
	CTable::cell_ptr_t CTable::CCell::GetMergePart() const
	{
		auto merge_part = std::make_shared<CTable::CCell>();
		merge_part->RecalcWithNewItem(this);
		merge_part->m_eVMerge		= CTable::CCell::eVMerge::vmContinue;
		merge_part->m_oTopBorder	= m_oTopBorder;
		merge_part->m_oLeftBorder	= m_oLeftBorder;
		merge_part->m_oBotBorder	= m_oBotBorder;
		merge_part->m_oRightBorder	= m_oRightBorder;
		merge_part->m_nGridSpan		= m_nGridSpan;
		merge_part->m_eShading		= m_eShading;
		merge_part->m_lColor		= m_lColor;
		return merge_part;
	}
	void CTable::CCell::AddParagraph(const paragraph_ptr_t& pParagraph, bool bIsGraphicalCell)
	{
		if (bIsGraphicalCell)
		{
			// top and right spacing added only when it first paragraph in cell
			if (m_arParagraphs.empty())
			{
				m_oTopBorder.dSpacing = pParagraph->m_dSpaceBefore;
				m_oRightBorder.dSpacing = pParagraph->m_dRightBorder;
			}
			// in ooxml table start from standart spacing (1.9), not from left cell border
			// therefore the left border is moved by this value
			pParagraph->m_dLeftBorder -= c_dSTANDART_TABLE_SPACING_MM;
			m_oBotBorder.dSpacing = pParagraph->m_dSpaceAfter;
		}

		m_arParagraphs.push_back(pParagraph);
	}

	void CTable::CRow::Clear()
	{
		m_arCells.clear();
	}
	void CTable::CRow::ToXml(NSStringUtils::CStringBuilder& oWriter) const
	{
		oWriter.WriteString(L"<w:tr>");

		oWriter.WriteString(L"<w:trPr>");
		oWriter.WriteString(L"<w:trHeight w:val=\"");
		oWriter.AddUInt(static_cast<unsigned int>(m_dHeight * c_dMMToDx));
		oWriter.WriteString(L"\" w:hRule=\"exact\"/>");
		oWriter.WriteString(L"</w:trPr>");

		for (const auto& c : m_arCells)
			c->ToXml(oWriter);

		oWriter.WriteString(L"</w:tr>");
	}
	void CTable::CRow::ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const
	{

	}
	void CTable::CRow::ToBin(NSWasm::CData& oWriter) const
	{

	}
	void CTable::CRow::AddCell(const cell_ptr_t& pCell)
	{
		if (m_arCells.empty())
		{
			m_dTop = pCell->m_dTop;
			m_dLeft = pCell->m_dLeft;
			m_dBot = pCell->m_dBot;
			m_dHeight = pCell->m_dHeight;
			m_dWidth = 0.0;
		}
		else if (pCell->m_dLeft - m_arCells.back()->m_dRight > c_dGRAPHICS_ERROR_MM)
		{
			auto last_cell = m_arCells.back();
			m_dWidth += pCell->m_dLeft - last_cell->m_dLeft - last_cell->m_dWidth;
			last_cell->m_dRight = pCell->m_dLeft;
			last_cell->m_dWidth = last_cell->m_dRight - last_cell->m_dLeft;
		}

		m_dRight = pCell->m_dRight;
		m_dWidth += pCell->m_dWidth;
		m_arCells.push_back(pCell);
	}
	bool CTable::CRow::IsEmpty() const
	{
		return m_arCells.empty();
	}
	CTable::cell_ptr_t CTable::CRow::GetLastCell() const noexcept
	{
		return m_arCells.back();
	}

	void CTable::Clear()
	{
		m_arRows.clear();
	}
	void CTable::ToXml(NSStringUtils::CStringBuilder& oWriter) const
	{
		oWriter.WriteString(L"<w:tbl>");

		oWriter.WriteString(L"<w:tblPr>");
		oWriter.WriteString(L"<w:tblpPr ");

		oWriter.WriteString(L"w:horzAnchor=\"page\" w:vertAnchor=\"page\" w:tblpX=\"");
		oWriter.AddInt64(static_cast<long long>((m_dLeft + c_dSTANDART_TABLE_SPACING_MM) * c_dMMToDx));
		oWriter.WriteString(L"\" w:tblpY=\"");
		oWriter.AddInt64(static_cast<long long>(m_dTop * c_dMMToDx));
		oWriter.WriteString(L"\" />");

		oWriter.WriteString(L"<w:tblW w:w=\"");
		oWriter.AddUInt(static_cast<unsigned int>(m_dWidth * c_dMMToDx));
		oWriter.WriteString(L"\" w:type=\"dxa\"/>");
		oWriter.WriteString(L"</w:tblPr>");

		oWriter.WriteString(L"<w:tblGrid>");
		for (const auto& gc : m_arGridCols)
		{
			oWriter.WriteString(L"<w:gridCol w:w=\"");
			oWriter.AddUInt(static_cast<unsigned int>(gc * c_dMMToDx));
			oWriter.WriteString(L"\" />");
		}
		oWriter.WriteString(L"</w:tblGrid>");

		for (const auto& r : m_arRows)
			r->ToXml(oWriter);

		oWriter.WriteString(L"</w:tbl>");
	}
	void CTable::ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const
	{

	}
	void CTable::ToBin(NSWasm::CData& oWriter) const
	{

	}
	void CTable::AddRow(const row_ptr_t& pRow)
	{
		if (m_arRows.empty())
		{
			m_dTop = pRow->m_dTop;
			m_dLeft = pRow->m_dLeft;
			m_dRight = pRow->m_dRight;
			m_dWidth = pRow->m_dWidth;
			m_dHeight = 0.0;

			for (const auto& c : pRow->m_arCells)
				m_arGridCols.push_back(c->m_dWidth);
		}

		if (pRow->m_arCells.size() > m_arGridCols.size())
		{
			m_arGridCols.clear();
			for (const auto& c : pRow->m_arCells)
				m_arGridCols.push_back(c->m_dWidth);
		}

		m_dBot = pRow->m_dBot;
		m_dHeight += pRow->m_dHeight;
		m_arRows.push_back(pRow);
	}
	void CTable::CalcGridCols()
	{
		// looking for rows with horizontally merged cells
		for (const auto& r : m_arRows)
		{
			int i = 0;
			auto size_diff = m_arGridCols.size() - r->m_arCells.size();
			// recalculation number of cell grids only if there are horizontally merged cells
			// the number of the cells is less than the number of columns
			if (size_diff)
			{
				for (const auto& c : r->m_arCells)
				{
					auto grids = m_arGridCols[i++];
					// calculate how many cells were merged into the current cell
					while (c->m_dWidth - grids > c_dCALCULATION_ERROR)
					{
						grids += m_arGridCols[i++];
						c->m_nGridSpan++;
						size_diff--;
					}
					if (!size_diff) break;
				}
			}
		}
	}
	bool CTable::IsEmpty() const
	{
		return m_arRows.empty();
	}
} // namespace NSDocxRenderer
