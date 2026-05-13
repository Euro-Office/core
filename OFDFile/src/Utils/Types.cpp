#include "Types.h"

#include "Utils.h"
#include "XmlReader.h"

namespace OFD
{
TBox::TBox()
	: m_dX(0.), m_dY(0.), m_dWidth(0.), m_dHeight(0.)
{}

bool TBox::Empty() const
{
	return m_dWidth < OFD_EPSILON || m_dHeight < OFD_EPSILON;
}

bool TBox::Read(const std::string& sValue)
{
	const std::vector<std::string> arValues{Split(sValue, ' ')};

	if (4 > arValues.size())
		return false;

	if (!StringToDouble(arValues[0], m_dX)     ||
	    !StringToDouble(arValues[1], m_dY)     ||
	    !StringToDouble(arValues[2], m_dWidth) ||
	    !StringToDouble(arValues[3], m_dHeight))
		return false;

	return true;
}

TMatrix::TMatrix()
	: m_dM11(1.), m_dM12(0.), m_dM21(0.), m_dM22(1.), m_dDx(0.), m_dDy(0.)
{}

bool TMatrix::Read(const std::string& sValue)
{
	const std::vector<std::string> arValues{Split(sValue, ' ')};

	if (6 > arValues.size())
		return false;

	if (!StringToDouble(arValues[0], m_dM11) ||
	    !StringToDouble(arValues[1], m_dM12) ||
	    !StringToDouble(arValues[2], m_dM21) ||
	    !StringToDouble(arValues[3], m_dM22) ||
	    !StringToDouble(arValues[4], m_dDx)  ||
	    !StringToDouble(arValues[5], m_dDy))
		return false;

	return true;
}

TPos::TPos()
	: m_dX(0.), m_dY(0.)
{}

bool TPos::Read(const std::string& sValue)
{
	const std::vector<std::string> arValues{Split(sValue, ' ')};

	if (2 > arValues.size())
		return false;

	if (!StringToDouble(arValues[0], m_dX) ||
	    !StringToDouble(arValues[1], m_dY))
		return false;

	return true;
}

TDest::TDest()
	: m_eType(EType::Fit), m_unPageID(0), m_pLeft(nullptr), m_pTop(nullptr),
	  m_pRight(nullptr), m_pBottom(nullptr), m_pZoom(nullptr)
{}

TDest::~TDest()
{
	if (nullptr != m_pLeft)
		delete m_pLeft;
	if (nullptr != m_pTop)
		delete m_pTop;
	if (nullptr != m_pRight)
		delete m_pRight;
	if (nullptr != m_pBottom)
		delete m_pBottom;
	if (nullptr != m_pZoom)
		delete m_pZoom;
}

TDest* TDest::Read(CXmlReader& oReader)
{
	if (!oReader.MoveToFirstAttribute())
		return nullptr;

	TDest *pDest = new TDest();

	if (nullptr == pDest)
	{
		oReader.MoveToElement();
		return nullptr;
	}

	std::string sName;

	do
	{
		sName = oReader.GetNameA();

		if ("Type" == sName)
		{
			const std::string sValue{oReader.GetTextA()};

			if ("XYZ" == sValue)
				pDest->m_eType = EType::XYZ;
			else if ("Fit" == sValue)
				pDest->m_eType = EType::Fit;
			else if ("FitH" == sValue)
				pDest->m_eType = EType::FitH;
			else if ("FitV" == sValue)
				pDest->m_eType = EType::FitV;
			else if ("FitR" == sValue)
				pDest->m_eType = EType::FitR;
		}
		else if ("PageID" == sName)
			pDest->m_unPageID = oReader.GetUInteger(true);
		else if ("Left" == sName)
			pDest->m_pLeft = new double{oReader.GetDouble(true)};
		else if ("Top" == sName)
			pDest->m_pTop = new double{oReader.GetDouble(true)};
		else if ("Right" == sName)
			pDest->m_pRight = new double{oReader.GetDouble(true)};
		else if ("Bottom" == sName)
			pDest->m_pBottom = new double{oReader.GetDouble(true)};
		else if ("Zoom" == sName)
			pDest->m_pZoom = new double{oReader.GetDouble(true)};
	}while(oReader.MoveToNextAttribute());

	oReader.MoveToElement();

	return pDest;
}
}
