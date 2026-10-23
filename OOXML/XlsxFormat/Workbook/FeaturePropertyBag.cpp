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
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */
#include "FeaturePropertyBag.h"

#include "../FileTypes_Spreadsheet.h"

#include "../../../DesktopEditor/common/File.h"
#include "../../../DesktopEditor/xml/include/xmlutils.h"
#include "../../Base/Unit.h"

namespace OOX
{
	namespace Spreadsheet
	{
		CFeaturePropertyBagFile::CFeaturePropertyBagFile(OOX::Document* pMain) : OOX::File(pMain)
		{
		}
		CFeaturePropertyBagFile::CFeaturePropertyBagFile(OOX::Document* pMain, const CPath& oRootPath, const CPath& oPath) : OOX::File(pMain)
		{
			read(oRootPath, oPath);
		}
		CFeaturePropertyBagFile::~CFeaturePropertyBagFile()
		{
		}
		void CFeaturePropertyBagFile::read(const CPath& oPath)
		{
			CPath oRootPath;
			read(oRootPath, oPath);
		}
		const OOX::FileType CFeaturePropertyBagFile::type() const
		{
			return OOX::Spreadsheet::FileTypes::FeaturePropertyBag;
		}
		const CPath CFeaturePropertyBagFile::DefaultDirectory() const
		{
			return type().DefaultDirectory();
		}
		const CPath CFeaturePropertyBagFile::DefaultFileName() const
		{
			return type().DefaultFileName();
		}
		void CFeaturePropertyBagFile::read(const CPath& oRootPath, const CPath& oPath)
		{
			XmlUtils::CXmlLiteReader oReader;

			if (!oReader.FromFile(oPath.GetPath()))
				return;
			if (!oReader.ReadNextNode())
				return;
			if (L"FeaturePropertyBags" != XmlUtils::GetNameNoNS(oReader.GetName()))
				return;
			if (oReader.IsEmptyNode())
				return;

			int nBagsDepth = oReader.GetDepth();
			while (oReader.ReadNextSiblingNode(nBagsDepth))
			{
				if (L"bag" != XmlUtils::GetNameNoNS(oReader.GetName()))
					continue;

				Bag oBag;
				if (oReader.MoveToFirstAttribute())
				{
					std::wstring wsName = oReader.GetName();
					while (!wsName.empty())
					{
						if (L"type" == wsName)
							oBag.sType = oReader.GetText();

						if (!oReader.MoveToNextAttribute())
							break;
						wsName = oReader.GetName();
					}
					oReader.MoveToElement();
				}
				if (false == oReader.IsEmptyNode())
				{
					int nBagDepth = oReader.GetDepth();
					while (oReader.ReadNextSiblingNode(nBagDepth))
					{
						std::wstring sName = XmlUtils::GetNameNoNS(oReader.GetName());
						if (L"bagId" == sName)
						{
							std::wstring sKey;
							if (oReader.MoveToFirstAttribute())
							{
								std::wstring wsName = oReader.GetName();
								while (!wsName.empty())
								{
									if (L"k" == wsName)
										sKey = oReader.GetText();

									if (!oReader.MoveToNextAttribute())
										break;
									wsName = oReader.GetName();
								}
								oReader.MoveToElement();
							}
							oBag.mapBagIds[sKey] = XmlUtils::GetInteger(oReader.GetText3());
						}
						else if (L"a" == sName)
						{
							if (oReader.IsEmptyNode())
								continue;

							int nArrayDepth = oReader.GetDepth();
							while (oReader.ReadNextSiblingNode(nArrayDepth))
							{
								if (L"bagId" == XmlUtils::GetNameNoNS(oReader.GetName()))
									oBag.arrMappedBagIds.push_back(XmlUtils::GetInteger(oReader.GetText3()));
							}
						}
					}
				}
				m_arrBags.push_back(oBag);
			}
		}
		bool CFeaturePropertyBagFile::IsCheckboxComplement(int nIndex) const
		{
			// xfComplement i -> XFComplements/MappedFeaturePropertyBags[i] -> XFComplement
			// -> XFControls -> CellControl -> bag type "Checkbox"
			const Bag* pComplements = NULL;
			for (size_t i = 0; i < m_arrBags.size(); ++i)
			{
				if (L"XFComplements" == m_arrBags[i].sType)
				{
					pComplements = &m_arrBags[i];
					break;
				}
			}
			if (NULL == pComplements || nIndex < 0 || nIndex >= (int)pComplements->arrMappedBagIds.size())
				return false;

			int nComplementId = pComplements->arrMappedBagIds[nIndex];
			if (nComplementId < 0 || nComplementId >= (int)m_arrBags.size())
				return false;

			const Bag& oComplement = m_arrBags[nComplementId];
			std::map<std::wstring, int>::const_iterator itControls = oComplement.mapBagIds.find(L"XFControls");
			if (oComplement.sType != L"XFComplement" || itControls == oComplement.mapBagIds.end())
				return false;

			int nControlsId = itControls->second;
			if (nControlsId < 0 || nControlsId >= (int)m_arrBags.size())
				return false;

			const Bag& oControls = m_arrBags[nControlsId];
			std::map<std::wstring, int>::const_iterator itControl = oControls.mapBagIds.find(L"CellControl");
			if (oControls.sType != L"XFControls" || itControl == oControls.mapBagIds.end())
				return false;

			int nControlId = itControl->second;
			if (nControlId < 0 || nControlId >= (int)m_arrBags.size())
				return false;

			return L"Checkbox" == m_arrBags[nControlId].sType;
		}
		void CFeaturePropertyBagFile::write(const CPath& oPath, const CPath& oDirectory, CContentTypes& oContent) const
		{
			// canonical content: a single Checkbox control chain; every checkbox xf
			// references it via <xfpb:xfComplement i="0"/>
			std::wstring sXml = L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
				L"<FeaturePropertyBags xmlns=\"http://schemas.microsoft.com/office/spreadsheetml/2022/featurepropertybag\">"
				L"<bag type=\"Checkbox\"/>"
				L"<bag type=\"XFControls\"><bagId k=\"CellControl\">0</bagId></bag>"
				L"<bag type=\"XFComplement\"><bagId k=\"XFControls\">1</bagId></bag>"
				L"<bag type=\"XFComplements\" extRef=\"XFComplementsMapperExtRef\">"
				L"<a k=\"MappedFeaturePropertyBags\"><bagId>2</bagId></a>"
				L"</bag>"
				L"</FeaturePropertyBags>";

			NSFile::CFileBinary::SaveToFile(oPath.GetPath(), sXml);

			oContent.Registration(type().OverrideType(), oDirectory, oPath.GetFilename());
		}
	}
} // namespace OOX
