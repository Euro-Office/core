/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at 20A-6 Ernesta Birznieka-Upish
 * street, Riga, Latvia, EU, LV-1050.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */
#pragma once

#include "Types.h"

namespace MathEquation
{
	struct TMathFont;
	class MathEquation;
	class IOutputDev
	{
	public:

		// Start and end of equation
		virtual void BeginEquation() = 0;
		virtual void EndEquation()   = 0;

		// Start and end of block.
		virtual void BeginBlock() = 0;
		virtual void EndBlock()   = 0;

		// Set size in current block
        virtual void SetSize(_UINT16 nSize) = 0;

		// Add symbol. Various elements can be added to symbol. Strikethrough, accent, etc.
        virtual void BeginChar(unsigned short uChar, unsigned char nTypeFace, bool bSpecialSymbol) = 0;
		virtual void AddCharEmbel(MEMBELTYPE eType) = 0;
		virtual void EndChar() = 0;

		// Matrix. Number of blocks here equals nRows * nCol, sent sequentially in normal order (first row left to right, second row left to right, etc.)
        virtual void BeginMatrix(unsigned char nVAlign, MMATRIXHORALIGN eHorAlign, MMATRIXVERALIGN eVerAlign, bool bEqualRows, bool bEqualCols, unsigned char nRows, unsigned char nCols, unsigned char* pVerBorders, unsigned char* pHorBorders) = 0;
		virtual void EndMatrix  () = 0;

        virtual void StartPile(unsigned char nHAlign, unsigned char nVAlign) = 0;
		virtual void EndPile() = 0;

		// Brackets with element inside
		virtual void BeginBrackets(MBRACKETSTYPE eType, bool bOpen, bool bClose) = 0;
		virtual void EndBrackets  (MBRACKETSTYPE eType, bool bOpen, bool bClose) = 0;

		// Root. First block - base, second if present - degree.
		virtual void BeginRoot(bool bDegree) = 0;
		virtual void EndRoot  () = 0;

		// Fractions. First block - numerator, second block - denominator.
		virtual void BeginFraction(MFRACTIONTYPES eType, bool bInline) = 0;
		virtual void EndFraction  () = 0;

		// If bInline = true, then block sequence is Base, Sub, Sup
		// If bInline = false, then block sequence is Sub, Sup, Base
		virtual void BeginScript(MSCRIPTALIGN eAlign, bool bBase = false, bool bSup = false, bool bSub = false, bool bInline = true) = 0;
		virtual void EndScript  () = 0;

		// Line above or below text
		virtual void BeginBar(MBARTYPE eType, bool bTop) = 0;
		virtual void EndBar  () = 0;

		// Arrow with label above or below (differs from BeginBar/EndBar in that here arrow is main
		// object, and text is small label, while those functions are opposite)
		virtual void BeginArrow(MARROWTYPE eType, bool bTop) = 0;
		virtual void EndArrow  () = 0;

		// Integrals. Block sequence is always following (if block present) Base, Sub, Sup
		virtual void BeginIntegral(MINTEGRALTYPE eType) = 0;
		virtual void EndIntegral  () = 0;

		// Curly brace with text above/below object. First comes main object, then curly brace label.
		virtual void BeginVerticalBrace(bool bTop) = 0;
		virtual void EndVerticalBrace  () = 0;

		// Sum, product, coproduct, union, intersection. Block sequence Base, Sub, Sup
		virtual void BeingNArray(MNARRAYTYPE eType) = 0;
		virtual void EndNArray  () = 0;

		// Long division. With quotient or without. First always comes base, then quotient if present.
		virtual void BeginLongDivision(MLONGDIVISION eType) = 0;
		virtual void EndLongDivision  () = 0;

		// < | > - this type of element
		virtual void BeginAngleBracketsWithSeparator(MANGLEBRACKETSWITHSEPARATORTYPE eType) = 0;
		virtual void EndAngleBracketsWithSeparator  () = 0;

        void AddFont(unsigned char nTypeFace, std::string sName, bool bBold, bool bItalic)
		{
			TMathFont aFont;
			aFont.sName   = sName;
			aFont.bBold   = bBold;
			aFont.bItalic = bItalic;
			m_mFonts[nTypeFace] = aFont;
		}

        TMathFont* GetFont(unsigned char nTypeFace)
		{
			TFontMap::iterator itFind = m_mFonts.find(nTypeFace);
			if (itFind != m_mFonts.end())			
				return &(itFind->second);

			return NULL;
		}

	protected:

		TFontMap m_mFonts;
	};

}
