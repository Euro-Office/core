#include "ImageObject.h"

#include "../../../DesktopEditor/graphics/Image.h"
#include "../../../OfficeUtils/src/ZipFolder.h"

namespace OFD
{
CImageObject::CImageObject(CXmlReader& oReader, IFolder* pFolder)
    : IPageBlock(oReader), CGraphicUnit(oReader), m_unMultiMediaID(0), m_pFolder(pFolder)
{
	if ("ofd:ImageObject" != oReader.GetNameA() || 0 == oReader.GetAttributesCount() || !oReader.MoveToFirstAttribute())
		return;

	std::string sAttributeName;

	do
	{
		sAttributeName = oReader.GetNameA();

		if ("ResourceID" == sAttributeName)
		{
			m_unMultiMediaID = oReader.GetUInteger(true);
			break;
		}
	} while(oReader.MoveToNextAttribute());

	oReader.MoveToElement();

	const int nDepth{oReader.GetDepth()};

	while(oReader.ReadNextSiblingNode2(nDepth))
	{
		ReadChildren(oReader);
	}
}

void CImageObject::Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const
{
	if (nullptr == pRenderer || nullptr == m_pFolder || nullptr == oCommonData.GetDocumentRes())
		return;

	const CMultiMedia* pMultiMedia = oCommonData.GetDocumentRes()->GetMultiMedia(m_unMultiMediaID);

	if (nullptr == pMultiMedia)
		return;

	TMatrix oOldTransform;
	CGraphicUnit::Apply(pRenderer, oOldTransform);

	const std::wstring wsFilePath = pMultiMedia->GetFilePath();

	if (wsFilePath.empty() || !m_pFolder->exists(wsFilePath))
		return;

	//TODO::It can be done without type determination. In all cases, just use Aggplus::CImage
	switch (m_pFolder->getType())
	{
		case IFolder::iftFolder:
		{
			pRenderer->DrawImageFromFile(m_pFolder->getFullFilePath(wsFilePath), 0, 0, 1, 1);
			break;
		}
		case IFolder::iftZip:
		{
			IFolder::CBuffer *pBuffer;
			if (m_pFolder->read(wsFilePath, pBuffer))
			{
				Aggplus::CImage oImage;
				oImage.Decode(pBuffer->Buffer, pBuffer->Size);
				delete pBuffer;

				pRenderer->DrawImage(&oImage, 0, 0, 1, 1);
			}
			break;
		}
	}

	pRenderer->SetTransform(oOldTransform.m_dM11, oOldTransform.m_dM12, oOldTransform.m_dM21, oOldTransform.m_dM22, oOldTransform.m_dDx, oOldTransform.m_dDy);
}

#ifdef BUILDING_WASM_MODULE
void CImageObject::GetLinks(NSWasm::CData& oRes) const
{
	CGraphicUnit::GetLinks(oRes);
}
#endif
}
