#pragma once
#include "BaseItem.h"
#include "TextLine.h"

namespace NSDocxRenderer
{
	class CParagraph : public CBaseItem, public IOoxmlItem
	{
	public:
		enum TextAlignmentType
		{
			tatUnknown,
			tatByLeft,
			tatByCenter,
			tatByRight,
			tatByWidth
		};

		// text frame properties
		bool                m_bIsNeedFirstLineIndent{false};
		bool                m_bIsShadingPresent     {false};
		LONG                m_lColorOfShadingFill   {c_iWhiteColor}; //BGR
		TextAlignmentType   m_eTextAlignmentType    {tatUnknown};

		// geometry paragraph
		double m_dLeftBorder {0.0}; // offset from left edge of page/shape/table
		double m_dRightBorder{0.0}; // offset from right edge of page/shape/table
		double m_dFirstLine  {0.0}; // offset relative to m_dLeftBorder

		double m_dSpaceBefore{0.0}; // default is 0 if w:before is absent
		double m_dSpaceAfter {0.0}; // in shape default is 8pt if w:after is absent
		double m_dLineHeight {0.0};

		std::vector<std::shared_ptr<CTextLine>> m_arTextLines;
		std::wstring m_wsStyleId;

	public:
		CParagraph() : CBaseItem() {}
		virtual ~CParagraph();
		virtual void Clear();
		virtual void ToXml(NSStringUtils::CStringBuilder& oWriter) const override final;
		virtual void ToXmlPptx(NSStringUtils::CStringBuilder& oWriter) const override final;
		virtual void ToBin(NSWasm::CData& oWriter) const override final;

		void RemoveHighlightColor();
		void MergeLines();
	};
}
