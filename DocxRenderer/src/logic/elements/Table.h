/*
 * Copyright (C) Ascensio System SIA, 2009-2026
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation, together with the
 * additional terms provided in the LICENSE file.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For
 * details, see the GNU AGPL at: https://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA by email at info@onlyoffice.com
 * or by postal mail at 20A-6 Ernesta Birznieka-Upisha Street, Riga,
 * LV-1050, Latvia, European Union.
 *
 * The interactive user interfaces in modified versions of the Program
 * are required to display Appropriate Legal Notices in accordance with
 * Section 5 of the GNU AGPL version 3.
 *
 * No trademark rights are granted under this License.
 *
 * All non-code elements of the Product, including illustrations,
 * icon sets, and technical writing content, are licensed under the
 * Creative Commons Attribution-ShareAlike 4.0 International License:
 * https://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 * This license applies only to such non-code elements and does not
 * modify or replace the licensing terms applicable to the Program's
 * source code, which remains licensed under the GNU Affero General
 * Public License v3.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include <vector>
#include <memory>

#include "BaseItem.h"
#include "Paragraph.h"
#include "Shape.h"

#include "../../resources/LinesTable.h"

namespace NSDocxRenderer
{
	class CGraphicalCell;
	class CTextCell;

	class CTable : public CBaseItem, public IOoxmlItem
	{
	public:
		class CCell;
		class CRow;

		using cell_ptr_t = std::shared_ptr<CCell>;
		using row_ptr_t = std::shared_ptr<CRow>;
		using paragraph_ptr_t = std::shared_ptr<CParagraph>;

	public:
		class CCell : public CBaseItem, public IOoxmlItem
		{
			friend class CTable;
		public:
			struct CBorder
			{
				double dWidth{};
				double dSpacing{};
				long lColor{};
				eLineType lineType{eLineType::ltNone};
			};

			enum class eVMerge
			{
				vmRestart,
				vmContinue
			};

			CCell() = default;
			CCell(const CCell& other);
			virtual ~CCell() = default;
			virtual void Clear();
			virtual void ToXml(NSStringUtils::CStringBuilder& oWriter) const override final;
			virtual void ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const override final;
			virtual void ToBin(NSWasm::CData& oWriter) const override final;

			CCell& operator=(const CCell& other);

			void AddParagraph(const paragraph_ptr_t& pParagraph);

			CBorder m_oBorderTop;
			CBorder m_oBorderBot;
			CBorder m_oBorderLeft;
			CBorder m_oBorderRight;

			unsigned int m_nGridSpan = 1;
			eVMerge m_eVMerge = CTable::CCell::eVMerge::vmRestart;

			std::vector<paragraph_ptr_t> m_arParagraphs;
		};
		class CRow : public CBaseItem, IOoxmlItem
		{
			friend class CTable;
		public:
			CRow() = default;
			virtual ~CRow() = default;
			virtual void Clear();
			virtual void ToXml(NSStringUtils::CStringBuilder& oWriter) const override final;
			virtual void ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const override final;
			virtual void ToBin(NSWasm::CData& oWriter) const override final;

			void AddCell(const cell_ptr_t& pCell);
			bool IsEmpty() const;

		private:
			std::vector<cell_ptr_t> m_arCells;
		};

		CTable() = default;
		virtual ~CTable() = default;
		virtual void Clear();
		virtual void ToXml(NSStringUtils::CStringBuilder& oWriter) const override final;
		virtual void ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const override final;
		virtual void ToBin(NSWasm::CData& oWriter) const override final;

		void AddRow(const row_ptr_t& pRow);
		void CalcGridCols();
		bool IsEmpty() const;

	private:
		std::vector<row_ptr_t> m_arRows;
		std::vector<double> m_arGridCols;
	};

	class CGraphicalCell : public CBaseItem
	{
	public:
		// realization
	};

	class CTextCell : public CBaseItem
	{
	public:
		void AddTextLine(const std::shared_ptr<CTextLine>& pTextLine);
		std::vector<std::shared_ptr<CTextLine>> m_arTextLines;

		double m_dMinPossibleTop = std::numeric_limits<double>::lowest();
		double m_dMinPossibleLeft = std::numeric_limits<double>::lowest();

		double m_dMaxPossibleBot = std::numeric_limits<double>::max();
		double m_dMaxPossibleRight = std::numeric_limits<double>::max();
	};
} // namespace NSDocxRenderer



