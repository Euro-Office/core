#ifndef PATHOBJECT_H
#define PATHOBJECT_H

#include "IPageBlock.h"
#include "GraphicUnit.h"

#include "../Types/Color.h"

namespace OFD
{
class IPathElement
{
public:
	IPathElement(){};
	virtual ~IPathElement(){};

	static IPathElement* ReadFromArray(std::vector<std::string>& arValues) { return nullptr; };
	virtual void Draw(IRenderer* pRenderer) const = 0;
};

class CStartElement : public IPathElement
{
	double m_dX;
	double m_dY;
public:
	CStartElement();
	static IPathElement* ReadFromArray(std::vector<std::string>::const_iterator& itBegin, const std::vector<std::string>::const_iterator& itEnd);
	void Draw(IRenderer* pRenderer) const override;
};

class CMoveElement : public IPathElement
{
	double m_dX;
	double m_dY;
public:
	CMoveElement();
	static CMoveElement* ReadFromArray(std::vector<std::string>::const_iterator& itBegin, const std::vector<std::string>::const_iterator& itEnd);
	static CMoveElement* ReadFromNode(CXmlReader& oReader);
	virtual void Draw(IRenderer* pRenderer) const override;
};

class CLineElement :  public IPathElement
{
	double m_dX;
	double m_dY;
public:
	CLineElement();
	static CLineElement* ReadFromArray(std::vector<std::string>::const_iterator& itBegin, const std::vector<std::string>::const_iterator& itEnd);
	static CLineElement* ReadFromNode(CXmlReader& oReader);
	void Draw(IRenderer* pRenderer) const override;
};

class CBezierCurve2Element : public IPathElement
{
	double m_dX1;
	double m_dY1;
	double m_dX2;
	double m_dY2;
public:
	CBezierCurve2Element();
	static CBezierCurve2Element* ReadFromArray(std::vector<std::string>::const_iterator& itBegin, const std::vector<std::string>::const_iterator& itEnd);
	static CBezierCurve2Element* ReadFromNode(CXmlReader& oReader);
	void Draw(IRenderer* pRenderer) const override;
};

class CBezierCurveElement : public IPathElement
{
	double m_dX1;
	double m_dY1;
	double m_dX2;
	double m_dY2;
	double m_dX3;
	double m_dY3;
public:
	CBezierCurveElement();
	static CBezierCurveElement* ReadFromArray(std::vector<std::string>::const_iterator& itBegin, const std::vector<std::string>::const_iterator& itEnd);
	static CBezierCurveElement* ReadFromNode(CXmlReader& oReader);
	void Draw(IRenderer* pRenderer) const override;
};

class CArcElement : public IPathElement
{
	double m_dRadiusX;
	double m_dRadiusY;
	double m_dAngle;
	bool   m_bLarge;
	bool   m_bSweep;
	double m_dX;
	double m_dY;
public:
	CArcElement();
	static CArcElement* ReadFromArray(std::vector<std::string>::const_iterator& itBegin, const std::vector<std::string>::const_iterator& itEnd);
	static CArcElement* ReadFromNode(CXmlReader& oReader);
	void Draw(IRenderer* pRenderer) const override;
};

class CCloseElement : public IPathElement
{
public:
	CCloseElement();
	void Draw(IRenderer* pRenderer) const override;
};

class CPathObject : public IPageBlock, public CGraphicUnit
{
	bool m_bStroke;
	bool m_bFill;

	enum class ERule
	{
		NonZero,
		Even_Odd
	} m_eRule;

	CColor* m_pFillColor;
	CColor* m_pStrokeColor;

	std::vector<const IPathElement*> m_arElements;

	void AddElement(const IPathElement* pElement);
public:
	CPathObject(CXmlReader& oReader);
	~CPathObject();

	void Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const override;

	#ifdef BUILDING_WASM_MODULE
	virtual void GetLinks(NSWasm::CData& oRes) const override;
	#endif
};
}

#endif // PATHOBJECT_H
