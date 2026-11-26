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
#include "Docx.h"

#include "App.h"
#include "Core.h"
#include "Document.h"
#include "FontTable.h"
#include "Numbering.h"
#include "Comments.h"
#include "Styles.h"
#include "Footnote.h"
#include "Endnote.h"
#include "Settings/Settings.h"
#include "Media/VbaProject.h"
#include "Media/JsaProject.h"
#include "HeaderFooter.h"
#include "CustomXml.h"

#include "../PPTXFormat/Theme.h"
#include "../../DesktopEditor/common/Directory.h"

namespace OOX
{
	void CDocx::_part_summary::init()
	{
		document = nullptr;
		fontTable = nullptr;
		numbering = nullptr;
		styles = nullptr;
		footnotes = nullptr;
		endnotes = nullptr;
		settings = nullptr;
		comments = nullptr;
	}

	CDocx::CDocx() : OOX::IFileContainer(dynamic_cast<OOX::Document*>(this))
	{
		init();
	}
	CDocx::CDocx(const CPath& oFilePath) : OOX::IFileContainer(this)
	{
		init();
		Read( oFilePath );
	}
	CDocx::~CDocx()
	{
		m_oMain.init();
		m_oGlossary.init();

		m_pApp = nullptr;
		m_pCore = nullptr;
		m_pTheme = nullptr;

		m_pCommentsExt = nullptr;
		m_pCommentsExtensible = nullptr;
		m_pCommentsIds = nullptr;
		m_pPeople = nullptr;
		m_pDocumentComments = nullptr;
		m_pDocumentCommentsExt = nullptr;
		m_pDocumentCommentsExtensible = nullptr;
		m_pDocumentPeople = nullptr;
		m_pDocumentCommentsIds = nullptr;
		m_pCommentsUserData = nullptr;

		m_pVbaProject = nullptr;
		m_pJsaProject = nullptr;
	}
	bool CDocx::Write(const CPath& oFilePath)
	{
		// Create a folder
        std::wstring filePath = oFilePath.GetPath();
        NSDirectory::CreateDirectory(filePath);

		OOX::CRels         oRels;
		OOX::CContentTypes oContent;

		IFileContainer::Write( oRels, oFilePath, OOX::CPath( L"" ), oContent );
		oRels.Write( oFilePath / FILE_SEPARATOR_STR );
		oContent.Write( oFilePath );

		return true;
	}
    bool CDocx::Read(const CPath& oFilePath)
    {
		m_sDocumentPath = oFilePath.GetPath();

        // Find "/_rels/.rels" and read all the files on rels
        OOX::CRels oRels( oFilePath / FILE_SEPARATOR_STR );
        IFileContainer::Read( oRels, oFilePath, oFilePath );

		FixAfterRead();
         return true;
	}
	void CDocx::FixAfterRead()
	{
		//solve id conflict between comments and documentComments
		if ((nullptr != m_oMain.comments && nullptr != m_pDocumentComments) && (!m_oMain.comments->m_arrComments.empty() && m_pDocumentComments->m_arrComments.empty()))
		{
			int maxId = INT_MIN;
			for (size_t i = 0; i < m_oMain.comments->m_arrComments.size(); ++i)
			{
				OOX::CComment* pComment = m_oMain.comments->m_arrComments[i];
				if (pComment->m_oId.IsInit() && maxId < pComment->m_oId->GetValue())
				{
					maxId = pComment->m_oId->GetValue();
				}
			}
			m_pDocumentComments->m_mapComments.clear();
			for (size_t i = 0; i < m_pDocumentComments->m_arrComments.size(); ++i)
			{
				OOX::CComment* pComment = m_pDocumentComments->m_arrComments[i];
				pComment->m_oId.Init();
				pComment->m_oId->SetValue(++maxId);
				m_pDocumentComments->m_mapComments.insert( std::make_pair( pComment->m_oId->GetValue(), i));
			}
		}
	}
	OOX::CHdrFtr *CDocx::GetHeaderOrFooter(const OOX::RId& rId, bool glossary) const
	{
		if (glossary && m_oGlossary.document)
		{
			OOX::IFileContainer* pDocumentContainer = (OOX::IFileContainer*)m_oGlossary.document;

			smart_ptr<OOX::File> pFile = pDocumentContainer->Find(rId);
			if (pFile.IsInit() && (OOX::FileTypes::Header == pFile->type() || OOX::FileTypes::Footer == pFile->type()))
				return (OOX::CHdrFtr*)pFile.GetPointer(); // NOLINT(clang-analyzer-cplusplus.NewDelete): the container map keeps the refcount alive
			else
				return nullptr;
		}
		else if (!glossary && m_oMain.document)
		{
			OOX::IFileContainer* pDocumentContainer = (OOX::IFileContainer*)m_oMain.document;

            smart_ptr<OOX::File> pFile = pDocumentContainer->Find( rId );
			if ( pFile.IsInit() && ( OOX::FileTypes::Header == pFile->type() || OOX::FileTypes::Footer == pFile->type() ) )
				return (OOX::CHdrFtr*)pFile.GetPointer(); // NOLINT(clang-analyzer-cplusplus.NewDelete): the container map keeps the refcount alive
			else 
				return nullptr;
		}
		
		return nullptr;
	}
	const std::wstring CDocx::GetCustomSettings() const
	{
		if (m_oMain.document)
		{
			std::vector<smart_ptr<OOX::File>>& container = m_oMain.document->GetContainer();
			for (size_t i = 0; i < container.size(); ++i)
			{
				if (OOX::FileTypes::CustomXml == container[i]->type())
				{
					OOX::CCustomXML* pCustomXml = dynamic_cast<OOX::CCustomXML*>(container[i].GetPointer());
					if(OOX::CSettingsCustom::GetSchemaUrl() == pCustomXml->GetSchemaUrl())
					{
						pCustomXml->m_bUsed = true;
						return NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)pCustomXml->m_sXmlA.c_str(), pCustomXml->m_sXmlA.length());
					}
				}
			}
		}
		return L"";
	}
	void CDocx::init()
	{
		m_oMain.init();
		m_oGlossary.init();

		m_pApp       = nullptr;
		m_pCore      = nullptr;
		m_pTheme     = nullptr;

		m_pCommentsExt	= nullptr;
		m_pCommentsExtensible	= nullptr;
		m_pCommentsIds	= nullptr;
		m_pPeople		= nullptr;
		m_pDocumentComments  = nullptr;
		m_pDocumentCommentsExt	= nullptr;
		m_pDocumentCommentsExtensible	= nullptr;
		m_pDocumentPeople		= nullptr;
		m_pDocumentCommentsIds	= nullptr;
		m_pCommentsUserData = nullptr;

		m_pVbaProject	= nullptr;
		m_pJsaProject	= nullptr;

		m_bGlossaryRead = false;
	}
}
