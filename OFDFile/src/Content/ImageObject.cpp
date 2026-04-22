#include "ImageObject.h"

#include "../../../DesktopEditor/graphics/Image.h"
#include "../../../OfficeUtils/src/ZipFolder.h"

namespace OFD
{
CImageObject::CImageObject(CXmlReader& oLiteReader, IFolder* pFolder)
	: IPageBlock(oLiteReader), CGraphicUnit(oLiteReader), m_unMultiMediaID(0), m_pFolder(pFolder)
{
	if ("ofd:ImageObject" != oLiteReader.GetNameA() || 0 == oLiteReader.GetAttributesCount() || !oLiteReader.MoveToFirstAttribute())
		return;

	std::string sAttributeName;

	do
	{
		sAttributeName = oLiteReader.GetNameA();

		if ("ResourceID" == sAttributeName)
		{
			m_unMultiMediaID = oLiteReader.GetUInteger(true);
			break;
		}
	} while(oLiteReader.MoveToNextAttribute());

	oLiteReader.MoveToElement();
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
}
