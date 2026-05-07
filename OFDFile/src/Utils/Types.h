#ifndef TYPES_H
#define TYPES_H

#include <string>

namespace OFD
{
#define OFD_EPSILON 0.0001
struct TBox
{
	double m_dX;
	double m_dY;
	double m_dWidth;
	double m_dHeight;

	TBox();

	bool Empty() const;
	bool Read(const std::string& wsValue);
};

struct TMatrix
{
	double m_dM11;
	double m_dM12;
	double m_dM21;
	double m_dM22;
	double m_dDx;
	double m_dDy;

	TMatrix();

	bool Read(const std::string& sValue);
};

struct TPos
{
	double m_dX;
	double m_dY;

	TPos();

	bool Read(const std::string& sValue);
};

class CXmlReader;

struct TDest
{
	enum class EType
	{
		XYZ,  // Go to a point with a specified zoom level (Left, Top, Zoom)
		Fit,  // Fit the whole page
		FitH, // Fit to width, scroll to top (Top)
		FitV, // Fit to height, scroll to Left (Left)
		FitR  // Fit the specified rectangle (Left, Top, Right, Bottom)
	}m_eType;

	unsigned int m_unPageID;

	double* m_pLeft;
	double* m_pTop;
	double* m_pRight;
	double* m_pBottom;
	double* m_pZoom;

	TDest();
	~TDest();

	static TDest* Read(CXmlReader& oReader);
};
}

#endif // TYPES_H
