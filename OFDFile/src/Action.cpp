#include "Action.h"

namespace OFD
{
void SetAction(const IAction*& pAction, IAction* pValue)
{
	if (nullptr != pAction)
		delete pAction;

	pAction = pValue;
}

CAction::CAction(CXmlReader& oReader)
	: m_eEvent(EEvent::CLICK), m_pRegion(nullptr), m_pAction(nullptr)
{
	if (oReader.MoveToFirstAttribute())
	{
		do
		{
			if ("Event" != oReader.GetNameA())
				continue;

			const std::wstring wsEvent{oReader.GetText()};

			if (L"Click" == wsEvent)
				m_eEvent = EEvent::CLICK;
			else if (L"Do" == wsEvent)
				m_eEvent = EEvent::DO;
			else if (L"Po" == wsEvent)
				m_eEvent = EEvent::PO;

		}while (oReader.MoveToNextAttribute());

		oReader.MoveToElement();
	}

	const int nDepth{oReader.GetDepth()};
	std::string sName;

	while (oReader.ReadNextSiblingNode(nDepth))
	{
		sName = oReader.GetNameA();

		if ("ofd:Goto" == sName)
			SetAction(m_pAction, new CGoto(oReader));
		else if ("ofd:URI" == sName)
			SetAction(m_pAction, new CURI(oReader));
		else if ("ofd:GotoA" == sName)
			SetAction(m_pAction, new CGotoA(oReader));
		else if ("ofd:Sound" == sName)
			SetAction(m_pAction, new CSound(oReader));
		else if ("ofd:Movie" == sName)
			SetAction(m_pAction, new CMovie(oReader));
		else
			continue;

		return;
	}
}

CAction::~CAction()
{
	if(nullptr != m_pRegion)
		delete m_pRegion;

	if (nullptr != m_pAction)
		delete m_pAction;
}

const IAction* CAction::GetAction() const
{
	return m_pAction;
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

TBookmark::TBookmark()
{}

TBookmark* TBookmark::Read(CXmlReader& oReader)
{
	if (!oReader.MoveToFirstAttribute())
		return nullptr;

	TBookmark* pBookmark = new TBookmark();

	if (nullptr == pBookmark)
	{
		oReader.MoveToElement();
		return nullptr;
	}

	do
	{
		if ("Name" == oReader.GetNameA())
			pBookmark->m_wsName = oReader.GetText();
	}while (oReader.MoveToNextAttribute());

	oReader.MoveToElement();

	return pBookmark;
}

CGoto::CGoto(CXmlReader& oReader)
	: m_pDest(nullptr), m_pBookmark(nullptr)
{
	const int nDepth{oReader.GetDepth()};

	while (oReader.ReadNextSiblingNode(nDepth))
	{
		if("ofd:Dest" == oReader.GetNameA())
		{
			TDest *pDest{TDest::Read(oReader)};

			if (nullptr == pDest)
				continue;

			if (nullptr != m_pDest)
				delete m_pDest;

			m_pDest = pDest;
		}
		else if ("ofd:Bookmark" == oReader.GetNameA())
		{
			TBookmark *pBookmark{TBookmark::Read(oReader)};

			if (nullptr == pBookmark)
				continue;

			if (nullptr != m_pBookmark)
				delete m_pBookmark;

			m_pBookmark = pBookmark;
		}
	}
}

CGoto::~CGoto()
{
	if (nullptr != m_pDest)
		delete m_pDest;

	if (nullptr != m_pBookmark)
		delete m_pBookmark;
}

EActionType CGoto::GetType() const
{
	return EActionType::Goto;
}

const TDest* CGoto::GetDest() const
{
	return m_pDest;
}

CGotoA::CGotoA(CXmlReader& oReader)
	: m_bNewWindow(false)
{
	if (!oReader.MoveToFirstAttribute())
		return;

	do
	{
		if ("AttachID" == oReader.GetNameA())
			m_unAttachID = oReader.GetUInteger(true);
		else if ("NewWindow" == oReader.GetNameA())
			m_bNewWindow = oReader.GetBoolean(true);
	}while(oReader.MoveToNextAttribute());

	oReader.MoveToElement();
}

CGotoA::~CGotoA()
{}

EActionType CGotoA::GetType() const
{
	return EActionType::GotoA;
}

CURI::CURI(CXmlReader& oReader)
{
	if (!oReader.MoveToFirstAttribute())
		return;

	do
	{
		if ("URI" == oReader.GetNameA())
			m_sURI = oReader.GetTextA();
		else if ("Base" == oReader.GetNameA())
			m_sBase = oReader.GetTextA();
	}while(oReader.MoveToNextAttribute());

	oReader.MoveToElement();
}

CURI::~CURI()
{}

EActionType CURI::GetType() const
{
	return EActionType::URI;
}

CSound::CSound(CXmlReader& oReader)
{
	if (!oReader.MoveToFirstAttribute())
		return;

	std::string sName;

	do
	{
		sName = oReader.GetNameA();

		if ("ResourceID" == sName)
			m_unResourceID = oReader.GetUInteger(true);
		else if ("Volume" == sName)
			m_nVolume = oReader.GetInteger(true);
		else if ("Repeat" == sName)
			m_bRepeat = oReader.GetBoolean(true);
		else if ("Synchronous" == sName)
			m_bSynchronous = oReader.GetBoolean(true);
	}while(oReader.MoveToNextAttribute());

	oReader.MoveToElement();
}

CSound::~CSound()
{}

EActionType CSound::GetType() const
{
	return EActionType::Sound;
}

CMovie::CMovie(CXmlReader& oReader)
{
	if (!oReader.MoveToFirstAttribute())
		return;

	do
	{
		if ("ResourceID" == oReader.GetNameA())
			m_unResourceID = oReader.GetUInteger(true);
		else if ("Operator" == oReader.GetNameA())
		{
			const std::string sValue{oReader.GetTextA()};

			if ("Play" == sValue)
				m_eOperator = EOperator::Play;
			else if ("Stop" == sValue)
				m_eOperator = EOperator::Stop;
			else if ("Pause" == sValue)
				m_eOperator = EOperator::Pause;
			else if ("Resume" == sValue)
				m_eOperator = EOperator::Resume;
		}
	}while(oReader.MoveToNextAttribute());

	oReader.MoveToElement();
}

CMovie::~CMovie()
{}

EActionType CMovie::GetType() const
{
	return EActionType::Movie;
}

}
