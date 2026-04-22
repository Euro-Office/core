#ifndef PENSETTINGS_H
#define PENSETTINGS_H

#include "../Utils/XmlReader.h"

class IRenderer;

namespace OFD
{
class CPenSettings
{
	double m_dLineWidth;

	enum class ECap
	{
		Butt,
		Round,
		Square
	} m_eCap;

	enum class EJoin
	{
		Miter,
		Round,
		Bevel
	} m_eJoin;

	double m_dMiterLimit;
	double m_dDashOffset;
	std::vector<double> m_arDashPattern;
	unsigned char m_uchAlpha;
public:
	CPenSettings(CXmlReader& oLiteReader);

	void Apply(IRenderer* pRenderer) const;
};
}

#endif // PENSETTINGS_H
