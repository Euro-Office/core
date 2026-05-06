#ifndef ACTION_H
#define ACTION_H

#include "Utils/XmlReader.h"
#include "Types/Region.h"

namespace OFD
{
enum class EActionType
{
	Goto,
	GotoA, // Transition to attachment
	URI,   // External link
	Sound, // Audio playback
	Movie  // Video playback
};

class IAction
{
public:
	IAction() = default;
	virtual ~IAction() = default;

	virtual EActionType GetType() const = 0;
};

class CAction
{
	enum class EEvent
	{
		DO,   //Opened document
		PO,   //Opened page
		CLICK
	} m_eEvent;

	CRegion* m_pRegion;
	const IAction *m_pAction;
public:
	CAction(CXmlReader& oReader);
	~CAction();

	const IAction* GetAction() const;
};

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

	UINT m_unPageID;

	double* m_pLeft;
	double* m_pTop;
	double* m_pRight;
	double* m_pBottom;
	double* m_pZoom;

	TDest();
	~TDest();

	static TDest* Read(CXmlReader& oReader);
};

struct TBookmark
{
	std::wstring m_wsName;

	TBookmark();

	static TBookmark* Read(CXmlReader& oReader);
};

class CGoto : public IAction
{
	const TDest*     m_pDest;
	const TBookmark* m_pBookmark;
public:
	CGoto(CXmlReader& oReader);
	~CGoto();

	EActionType GetType() const override;

	const TDest* GetDest() const;
};

class CGotoA : public IAction
{
	UINT m_unAttachID;
	bool m_bNewWindow;
public:
	CGotoA(CXmlReader& oReader);
	~CGotoA();

	EActionType GetType() const override;
};

class CURI : public IAction
{
	std::string m_sURI;
	std::string m_sBase;
public:
	CURI(CXmlReader& oReader);
	~CURI();

	EActionType GetType() const override;
};

class CSound : public IAction
{
	UINT m_unResourceID;
	int  m_nVolume;
	bool m_bRepeat;
	bool m_bSynchronous;
public:
	CSound(CXmlReader& oReader);
	~CSound();

	EActionType GetType() const override;
};

class CMovie : public IAction
{
	UINT m_unResourceID;
	enum class EOperator
	{
		Play,
		Stop,
		Pause,
		Resume
	} m_eOperator;
public:
	CMovie(CXmlReader& oReader);
	~CMovie();

	EActionType GetType() const override;
};
}

#endif // ACTION_H
