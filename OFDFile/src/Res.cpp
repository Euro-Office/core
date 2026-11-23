#include "Res.h"

#include "Utils/Utils.h"

#include "../../DesktopEditor/common/Directory.h"

namespace OFD
{
CRes::CRes()
{}

CRes::~CRes()
{
	#define CLEAR_VECTOR(arvaribles)\
	for (const auto& oValue : (arvaribles))\
		delete oValue.second

	CLEAR_VECTOR(m_mColorSpaces);
	CLEAR_VECTOR(m_mDrawParams);
	CLEAR_VECTOR(m_mFonts);
	CLEAR_VECTOR(m_mMultiMedias);
	CLEAR_VECTOR(m_mCCompositeGraphicUnits);
}

template<class T>
inline void AddElementToMap(T* pElement, unsigned int unIndex, std::map<unsigned int, T*>& mElements)
{
	if (nullptr == pElement)
		return;

	typename std::map<unsigned int, T*>::const_iterator itFound = mElements.find(unIndex);

	if (mElements.cend() != itFound)
		delete itFound->second;

	mElements.insert(std::make_pair(unIndex, pElement));
}

bool CRes::Read(const std::wstring& wsFilePath, const std::wstring& wsRootPath)
{
	if (wsFilePath.empty() || !CanUseThisPath(wsFilePath, wsRootPath))
		return false;

	const std::wstring wsFullPath{CombinePaths(wsRootPath, wsFilePath)};

	CXmlReader oLiteReader;
	if (!oLiteReader.FromFile(wsFullPath) || !oLiteReader.ReadNextNode() || L"ofd:Res" != oLiteReader.GetName() || oLiteReader.IsEmptyNode())
		return false;

	std::wstring wsResRootPath{wsRootPath};

	if (0 != oLiteReader.GetAttributesCount() && oLiteReader.MoveToFirstAttribute())
	{
		std::string sNodeName;
		do
		{
			sNodeName = oLiteReader.GetNameA();

			if ("BaseLoc" == sNodeName)
			{
				wsResRootPath = CombinePaths(NSDirectory::GetFolderPath(wsFullPath), oLiteReader.GetText());
				break;
			}
		} while (oLiteReader.MoveToNextAttribute());
	}

	oLiteReader.MoveToElement();

	std::string sNodeName;

	#define PARSE_CONTAINER(container_name, element_name, melements, creator)\
	if ((container_name) == sNodeName)\
	{\
		const int nChildrenDepth = oLiteReader.GetDepth();\
		while (oLiteReader.ReadNextSiblingNode(nChildrenDepth))\
		{\
			if ((element_name) == oLiteReader.GetNameA())\
			{\
				auto* pElement = creator;\
				AddElementToMap(pElement, pElement->GetID(), melements);\
			}\
		}\
		continue;\
	}

	#define PARSE_CONTAINER_WITHOUT_PATH(container_name, element_name, element_type, melements)\
	PARSE_CONTAINER(container_name, element_name, melements, new element_type(oLiteReader))

	#define PARSE_CONTAINER_WITH_PATH(container_name, element_name, element_type, melements)\
	PARSE_CONTAINER(container_name, element_name, melements, new element_type(oLiteReader, wsResRootPath))

	const int nDepth = oLiteReader.GetDepth();

	while (oLiteReader.ReadNextSiblingNode(nDepth))
	{
		sNodeName = oLiteReader.GetNameA();

		PARSE_CONTAINER_WITHOUT_PATH("ofd:ColorSpaces",           "ofd:ColorSpace",            CColorSpace,           m_mColorSpaces)
		PARSE_CONTAINER_WITHOUT_PATH("ofd:DrawParams",            "ofd:DrawParam",             CDrawParam,            m_mDrawParams)
		PARSE_CONTAINER_WITHOUT_PATH("ofd:CompositeGraphicUnits", "ofd:CCompositeGraphicUnit", CCompositeGraphicUnit, m_mCCompositeGraphicUnits)

		PARSE_CONTAINER_WITH_PATH("ofd:Fonts",                    "ofd:Font",                  CFont,                 m_mFonts)
		PARSE_CONTAINER_WITH_PATH("ofd:MultiMedias",              "ofd:MultiMedia",            CMultiMedia,           m_mMultiMedias)
	}

	return true;
}

#define RETURN_ELEMENT_FROM_MAP(mElements)\
	const auto itFound = (mElements).find(unId);\
	return ((mElements).cend() != itFound) ? itFound->second : nullptr\

const CColorSpace* CRes::GetColorSpace(unsigned int unId) const
{
	RETURN_ELEMENT_FROM_MAP(m_mColorSpaces);
}

const CDrawParam* CRes::GetDrawParam(unsigned int unId) const
{
	RETURN_ELEMENT_FROM_MAP(m_mDrawParams);
}

const CFont* CRes::GetFont(unsigned int unId) const
{
	RETURN_ELEMENT_FROM_MAP(m_mFonts);
}

const CMultiMedia* CRes::GetMultiMedia(unsigned int unId) const
{
	RETURN_ELEMENT_FROM_MAP(m_mMultiMedias);
}

const CCompositeGraphicUnit* CRes::GetCompositeGraphicUnit(unsigned int unId) const
{
	RETURN_ELEMENT_FROM_MAP(m_mCCompositeGraphicUnits);
}

std::vector<const CDrawParam*> CRes::GetDrawParams() const
{
	std::vector<const CDrawParam*> arValues;

	for (std::map<unsigned int, CDrawParam*>::const_iterator itBegin = m_mDrawParams.cbegin(); itBegin != m_mDrawParams.cend(); ++itBegin)
		arValues.push_back(itBegin->second);

	return arValues;
}
}
