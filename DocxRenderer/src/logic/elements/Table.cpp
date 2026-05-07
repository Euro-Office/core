#include "Table.h"

#include "../../resources/SingletonTemplate.h"
#include "../../resources/utils.h"

#include <iterator>

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
	bool CTable::CCell::IsPosibleToDivide() const noexcept
	{
		if (m_arParagraphs.size() < 2)
			return false;

		std::set<double> hor_lines, right_lines, left_lines;
		for (const auto& p : m_arParagraphs)
		{
			left_lines.insert(p->m_dLeft);
			right_lines.insert(p->m_dRight);
			hor_lines.insert({p->m_dTop, p->m_dBot});
		}

		auto is_eq = makeEqualComp<double>(c_dGRAPHICS_ERROR_MM);
		auto clear_near_lines = [&is_eq] (std::set<double>& lines) {
			for (auto it = lines.begin(); it != lines.end() && std::next(it) != lines.end(); )
				if (is_eq(*it, *std::next(it)))
					it = lines.erase(it);
				else
					++it;
		};

		clear_near_lines(hor_lines);
		clear_near_lines(right_lines);
		clear_near_lines(left_lines);

		if (hor_lines.size() == 2)
			return left_lines.size() + right_lines.size() == 2 * m_arParagraphs.size();
		else if (left_lines.size() == 1 && right_lines.size() == 1)
			return true;
		else if (left_lines.size() == 1 && right_lines.size() > 1)
			return false;
		else
			return true;
	}
	std::vector<CTable::cell_ptr_t> CTable::CCell::GetSubCells(const std::set<double>& graphical_hor_lines)
	{
		auto is_eq = makeEqualComp<double>(c_dGRAPHICS_ERROR_MM);

		// check if the lines form an empty row or column
		auto check_empty_space = [&is_eq, this] (const double& left, const double& right) -> bool {
			for (const auto& p : m_arParagraphs)
				if ((left < p->m_dLeft	 || is_eq(left, p->m_dLeft)) &&
					(right > p->m_dRight || is_eq(right, p->m_dRight)))
					return false;
			return true;
		};

		// a function that produces vertical and horizontal lines (depend on arg) from paragraphs
		// these lines will divide the cell into subcells
		// the number of which is equal to the multiplication
		// of the vertical and horizontal lines minus one
		//
		// the point is that we get the minimum number of table cells
		// that will fit all the paragraphs
		//
		// [p][-][-][-]
		// [-][-][p][-]
		// [-][p][-][-]
		// [-][-][-][p]
		//
		// (only 4 paragraphs, but 16 cells)
		auto get_lines = [&is_eq, &check_empty_space, this] (bool hor) -> std::set<double> {
			std::set<double> lines;
			lines.insert(hor ? m_dTop : m_dLeft);

			// main loop - check that all paragraphs have been used
			for(size_t i = 0; i < m_arParagraphs.size();)
			{
				auto cur_line = *lines.crbegin();
				std::set<double> tmp_lines, new_lines;
				for (const auto& p : m_arParagraphs)
					// for the current bottom/right line added to the set,
					// select all bot lines of paaragraphs
					// that have such a top/left line
					if (is_eq(hor ? p->m_dTop : p->m_dLeft, cur_line + (hor ? 0 : c_dSTANDART_TABLE_SPACING_MM)))
					{
						tmp_lines.insert(hor ? p->m_dBot : p->m_dRight);
						// mark that have checked this paragraph
						i++;
					}
					// add all the remaining paragraphs to the set of potential new lines
					// for the case when there is a gap between consecutive paragraphs of the line,
					// need to add a new line for the following cells
					else
						new_lines.insert(hor ? p->m_dTop : p->m_dLeft);

				if (tmp_lines.empty())
					// if among the remaining paragraphs there is no paragraph
					// that is below the current line -> exit the loop
					if (new_lines.upper_bound(cur_line) == new_lines.end())
						break;
					// if there is empty space between cells -> attach it to the previous cells
					// delete the previous border and add a new one
					else
					{
						auto new_line = *new_lines.upper_bound(cur_line);
						lines.insert(new_line - (hor ? 0.0 : c_dSTANDART_TABLE_SPACING_MM));
						if (check_empty_space(cur_line, new_line))
							lines.erase(cur_line);
					}
				else
					// end-of-boundary check
					if (is_eq(*tmp_lines.crbegin(), hor ? m_dBot : m_dRight))
						lines.insert(hor ? m_dBot : m_dRight);
					else
						lines.insert(*tmp_lines.crbegin());
			}

			// add the right and bot borders, respectively, in case
			// there is still a small gap before the graphic border
			auto last_line = *lines.crbegin();
			lines.insert(hor ? m_dBot : m_dRight);
			if (!is_eq(last_line, *lines.crbegin()) && check_empty_space(last_line, *lines.crbegin()))
				lines.erase(last_line);

			return lines;
		};

		auto hor_lines = get_lines(true);
		auto ver_lines = get_lines(false);

		for (auto non_gr_it = hor_lines.begin(), gr_it = graphical_hor_lines.begin(); non_gr_it != hor_lines.end() && gr_it != graphical_hor_lines.end();)
		{
			const auto& non_gr_val = *non_gr_it;
			const auto& gr_val = *gr_it;
			if (is_eq(non_gr_val, gr_val))
			{
				auto inserted = hor_lines.insert(gr_val).second;
				if (inserted)
				{
					non_gr_it = hor_lines.find(non_gr_val);
					non_gr_it = hor_lines.erase(non_gr_it);
				}
				++non_gr_it;
				++gr_it;
			}
			else if (*non_gr_it > *gr_it)
				++gr_it;
			else
				++non_gr_it;
		}

		std::vector<cell_ptr_t> non_graphical_cells;
		non_graphical_cells.reserve((hor_lines.size() - 1) * (ver_lines.size() - 1));

		// create cells from the resulting horizontal and vertical lines
		// add graphic borders to the outer cells
		for (auto hl_it = hor_lines.cbegin(); std::next(hl_it) != hor_lines.cend(); ++hl_it)
			for (auto vl_it = ver_lines.cbegin(); std::next(vl_it) != ver_lines.cend(); ++vl_it)
			{
				auto cell = std::make_shared<CCell>(
					*vl_it, *hl_it, *std::next(vl_it), *std::next(hl_it),
					vl_it == ver_lines.cbegin() ? m_oLeftBorder : CBorder(),
					hl_it == hor_lines.cbegin() ? m_oTopBorder : CBorder(),
					std::next(vl_it) == std::prev(ver_lines.cend()) ? m_oRightBorder : CBorder(),
					std::next(hl_it) == std::prev(hor_lines.cend()) ? m_oBotBorder : CBorder()
					);
				non_graphical_cells.emplace_back(std::move(cell));
			}

		for (auto& c : non_graphical_cells)
			for (auto& p : m_arParagraphs)
				// add a paragraph to a cell if the paragraph boundaries are within the cell boundaries
				if ((c->m_dTop < p->m_dTop		|| is_eq(c->m_dTop, p->m_dTop)) &&
					(c->m_dLeft < p->m_dLeft	|| is_eq(c->m_dLeft, p->m_dLeft)) &&
					(c->m_dBot > p->m_dBot		|| is_eq(c->m_dBot, p->m_dBot)) &&
					(c->m_dRight > p->m_dRight	|| is_eq(c->m_dRight, p->m_dRight)))
				{
					p->m_dLeftBorder = c_dSTANDART_TABLE_SPACING_MM;
					p->m_dSpaceBefore = 0;
					c->AddParagraph(std::move(p));
				}

		return non_graphical_cells;
	}
	void CTable::CCell::AddParagraph(const paragraph_ptr_t& pParagraph)
	{
		// check if a cell has no visible borders
		// dont need to set/update
		//
		// top and right spacing added only when it first paragraph in cell
		if (m_arParagraphs.empty())
		{
			if (m_oTopBorder.lineType != eLineType::ltNone)
				m_oTopBorder.dSpacing = pParagraph->m_dSpaceBefore;
			if (m_oRightBorder.lineType != eLineType::ltNone)
				m_oRightBorder.dSpacing = pParagraph->m_dRightBorder;
		}
		// in ooxml table start from standart spacing (1.9), not from left cell border
		// therefore the left border is moved by this value
		pParagraph->m_dLeftBorder -= c_dSTANDART_TABLE_SPACING_MM;
		if (m_oBotBorder.lineType != eLineType::ltNone)
			m_oBotBorder.dSpacing = pParagraph->m_dSpaceAfter;
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

		m_dRight = pCell->m_dRight;
		m_dWidth += pCell->m_dWidth;
		m_arCells.push_back(pCell);
	}
	bool CTable::CRow::IsEmpty() const
	{
		return m_arCells.empty();
	}
	void CTable::CRow::MergeRows(row_ptr_t row1, row_ptr_t row2) noexcept
	{
		// need to update right and width for last cell in row before merge
		row1->m_arCells.back()->m_dRight = row2->m_arCells.front()->m_dLeft;
		row1->m_dWidth += row1->m_arCells.back()->m_dRight - row1->m_arCells.back()->m_dLeft - row1->m_arCells.back()->m_dWidth;
		row1->m_arCells.back()->m_dWidth = row1->m_arCells.back()->m_dRight - row1->m_arCells.back()->m_dLeft;
		for (auto& c : row2->m_arCells)
			row1->AddCell(std::move(c));
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
		}

		UpdateGrids(pRow);

		m_dBot = pRow->m_dBot;
		m_dHeight += pRow->m_dHeight;
		m_arRows.push_back(pRow);
	}
	void CTable::UpdateGrids(row_ptr_t pRow)
	{
		if (!pRow || pRow->m_arCells.empty())
			return;

		// set first grids
		if (m_arGridCols.empty())
		{
			for (const auto& c : pRow->m_arCells)
				m_arGridCols.push_back(c->m_dWidth);
			return;
		}

		std::vector<double> widths;

		for (const auto& c : pRow->m_arCells)
			widths.push_back(c->m_dWidth);

		auto update = [this, &pRow, &widths] (std::vector<double>::iterator& it1, std::vector<double>::iterator& it2) {
			auto is_eq = makeEqualComp<double>(c_dGRAPHICS_ERROR_MM);

			if (!is_eq(*it1, *it2))
			{
				// define the type of merger
				//
				// First variant (greater = true)
				//
				// [...][k  k+1][...]
				// [...][k][k+1][...]
				//
				// Second variant (grater = false)
				//
				// [...][k][k+1][...]
				// [...][k  k+1][...]
				bool greater = (*it2 - *it1) > 0;
				auto& it_to_compare = greater ? it1 : it2;
				auto& it_to_update = greater ? it2 : it1;

				unsigned int grid_count = 1;
				std::vector<double> new_grids;
				auto grid_sum = [&new_grids] () -> double {
					double sum = 0.0;
					for (const auto& g : new_grids)
						sum += g;
					return sum;
				};

				new_grids.push_back(*it_to_compare++);

				// collect the width until the sum of the cell withs becomes equal to the merged cell
				//
				// First variant (greater = true)
				//
				// widths from current row cells (vector widths) to new_grids,
				// comare with width of table column (m_arGridCol)
				//
				// Second variant (greater = false)
				//
				// widths from table column widths (m_arGridCol),
				// compare with cell width from current row
				for (; it_to_compare != (greater ? widths.end() : m_arGridCols.end()); ++it_to_compare)
				{
					if (is_eq(*it_to_update, grid_sum()))
						break;
					new_grids.push_back(*it_to_compare);
					grid_count++;
				}

				size_t index = std::distance(greater ? m_arGridCols.begin() : widths.begin(), it_to_update);

				// First variant (greater = true)
				//
				// update the table column widths -
				// add new widths and remove the old one,
				// update the GridSpan for all required cells
				// in the previous rows
				//
				// Second variant (greater = false)
				//
				// update GridSpan for required cell from current row
				if (greater)
				{
					it_to_update = m_arGridCols.erase(it_to_update);
					auto insert_it = m_arGridCols.insert(it_to_update, new_grids.begin(), new_grids.end());
					it_to_update = std::next(insert_it, grid_count);

					for (auto& r : m_arRows)
					{
						// get the corret index for each row
						//
						// needed for the case when the previous row
						// had a merged cells before the index,
						// and in the current row these cells are not merged
						//
						//
						// [1  2][3  4][...] - in this row index need to be 2, not 3
						// [1][2][3  4][...] - in this row index also 3
						// [1][2][3][4][...]
						//        ^
						//        |
						// get update index for previous rows - 3
						auto correct_index = index;
						for (auto i = 0; i < index + 1; i++)
							correct_index -= r->m_arCells[i]->m_nGridSpan - 1;
						r->m_arCells.at(correct_index)->m_nGridSpan = grid_count;
					}
				}
				else
				{
					pRow->m_arCells.at(index)->m_nGridSpan = grid_count;
					++it_to_update;
				}
			}
			else
			{
				++it1;
				++it2;
			}
		};

		// three main cases:
		//
		// 1. if the new row has merged cells in it
		// 2. if the previous rows in table had merged cells
		// 3. if 1 and 2 together
		for (auto it1 = widths.begin(), it2 = m_arGridCols.begin(); it1 != widths.end() && it2 != m_arGridCols.end(); )
			update(it1, it2);
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
					while (c->m_dWidth - grids > c_dCOMPARE_EPSILON)
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
	bool CTable::MergeTables(std::shared_ptr<CTable> table1, std::shared_ptr<CTable> table2) noexcept
	{
		if (table1->m_arRows.size() == table2->m_arRows.size())
		{
			for (auto it1 = table1->m_arRows.begin(), it2 = table2->m_arRows.begin(); it1 != table1->m_arRows.end(); ++it1, ++it2)
			{
				CRow::MergeRows(*it1, *it2);
				// need to update grids, because the number of cells has increased
				table1->UpdateGrids(*it1);
			}

			// update right and width after merge
			table1->m_dRight = table1->m_arRows.front()->m_dRight;
			table1->m_dWidth = table1->m_arRows.front()->m_dWidth;

			table1->CalcGridCols();
			return true;
		}
		return false;
	}
} // namespace NSDocxRenderer
