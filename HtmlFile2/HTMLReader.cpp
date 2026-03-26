#include "HTMLReader.h"

#include "../Common/Network/FileTransporter/include/FileTransporter.h"
#include "../DesktopEditor/common/Path.h"

#include "../Common/3dParty/html/htmltoxhtml.h"

#include "Common.h"

#include "Writers/OOXMLWriter.h"
#include "Tags/OOXMLTags.h"

#include "Writers/MDWriter.h"
#include "Tags/MDTags.h"

#include "../Common/3dParty/html/gumbo-parser/src/gumbo.h"
#include "src/StringFinder.h"

#include <boost/tuple/tuple.hpp>

#include "../DesktopEditor/common/Directory.h"
#include "../DesktopEditor/common/File.h"

namespace HTML
{
#define HTML_TAG(tag) GUMBO_TAG_##tag
#define ADD_TAG(strName, enumName) {strName, HTML_TAG(enumName)}
#define SKIP_TAG SCRIPT
#define UNKNOWN_TAG GumboTag::GUMBO_TAG_UNKNOWN
#define HtmlTag GumboTag

#define TAGS_COUNT 32

const static std::map<std::wstring, HtmlTag> m_HTML_TAGS
{
	ADD_TAG(L"a", A),
	ADD_TAG(L"abbr", ABBR),
	ADD_TAG(L"acronym", ACRONYM),
	ADD_TAG(L"address", ADDRESS),
	ADD_TAG(L"applet", APPLET),
	ADD_TAG(L"area", AREA),
	ADD_TAG(L"article", ARTICLE),
	ADD_TAG(L"aside", ASIDE),
	ADD_TAG(L"audio", AUDIO),
	ADD_TAG(L"b", B),
	ADD_TAG(L"base", BASE),
	ADD_TAG(L"basefont", BASEFONT),
	ADD_TAG(L"bdi", BDI),
	ADD_TAG(L"bdo", BDO),
	ADD_TAG(L"bgsound", BGSOUND),
	ADD_TAG(L"blockquote", BLOCKQUOTE),
	ADD_TAG(L"big", BIG),
	ADD_TAG(L"body", BODY),
	ADD_TAG(L"blink", BLINK),
	ADD_TAG(L"br", BR),
	ADD_TAG(L"button", BUTTON),
	ADD_TAG(L"canvas", CANVAS),
	ADD_TAG(L"caption", CAPTION),
	ADD_TAG(L"center", CENTER),
	ADD_TAG(L"cite", CITE),
	ADD_TAG(L"code", CODE),
	ADD_TAG(L"col", COL),
	ADD_TAG(L"colgroup", COLGROUP),
	ADD_TAG(L"command", SKIP_TAG), // Данного обозначения нет, но т.к.мы всё равно пропускаем, то делаем script
	ADD_TAG(L"comment", SKIP_TAG), // Данного обозначения нет, но т.к.мы всё равно пропускаем, то делаем script
	ADD_TAG(L"datalist", DATALIST),
	ADD_TAG(L"dd", DD),
	ADD_TAG(L"del", DEL),
	ADD_TAG(L"details", DETAILS),
	ADD_TAG(L"dfn", DFN),
	ADD_TAG(L"dir", DIR),
	ADD_TAG(L"div", DIV),
	ADD_TAG(L"dl", DL),
	ADD_TAG(L"dt", DT),
	ADD_TAG(L"em", EM),
	ADD_TAG(L"embed", EMBED),
	ADD_TAG(L"fieldset", FIELDSET),
	ADD_TAG(L"figcaption", FIGCAPTION),
	ADD_TAG(L"figure", FIGURE),
	ADD_TAG(L"font", FONT),
	ADD_TAG(L"form", FORM),
	ADD_TAG(L"footer", FOOTER),
	ADD_TAG(L"frame", FRAME),
	ADD_TAG(L"frameset", FRAMESET),
	ADD_TAG(L"h1", H1),
	ADD_TAG(L"h2", H2),
	ADD_TAG(L"h3", H3),
	ADD_TAG(L"h4", H4),
	ADD_TAG(L"h5", H5),
	ADD_TAG(L"h6", H6),
	ADD_TAG(L"head", HEAD),
	ADD_TAG(L"header", HEADER),
	ADD_TAG(L"hgroup", HGROUP),
	ADD_TAG(L"hr", HR),
	ADD_TAG(L"html", HTML),
	ADD_TAG(L"i", I),
	ADD_TAG(L"iframe", IFRAME),
	ADD_TAG(L"img", IMG),
	ADD_TAG(L"input", INPUT),
	ADD_TAG(L"ins", INS),
	ADD_TAG(L"isindex", ISINDEX),
	ADD_TAG(L"kbd", KBD),
	ADD_TAG(L"keygen", KEYGEN),
	ADD_TAG(L"label", LABEL),
	ADD_TAG(L"legend", LEGEND),
	ADD_TAG(L"li", LI),
	ADD_TAG(L"link", LINK),
	ADD_TAG(L"main", MAIN),
	ADD_TAG(L"map", MAP),
	ADD_TAG(L"marquee", MARQUEE),
	ADD_TAG(L"mark", MARK),
	ADD_TAG(L"menu", MENU),
	ADD_TAG(L"meta", META),
	ADD_TAG(L"meter", METER),
	ADD_TAG(L"nav", NAV),
	ADD_TAG(L"nobr", NOBR),
	ADD_TAG(L"noembed", NOEMBED),
	ADD_TAG(L"noframes", NOFRAMES),
	ADD_TAG(L"noscript", NOSCRIPT),
	ADD_TAG(L"object", OBJECT),
	ADD_TAG(L"ol", OL),
	ADD_TAG(L"optgroup", OPTGROUP),
	ADD_TAG(L"option", OPTION),
	ADD_TAG(L"output", OUTPUT),
	ADD_TAG(L"p", P),
	ADD_TAG(L"param", PARAM),
	ADD_TAG(L"plaintext", PLAINTEXT),
	ADD_TAG(L"pre", PRE),
	ADD_TAG(L"progress", PROGRESS),
	ADD_TAG(L"q", Q),
	ADD_TAG(L"rp", RP),
	ADD_TAG(L"rt", RT),
	ADD_TAG(L"ruby", RUBY),
	ADD_TAG(L"s", S),
	ADD_TAG(L"samp", SAMP),
	ADD_TAG(L"script", SCRIPT),
	ADD_TAG(L"section", SECTION),
	ADD_TAG(L"select", SELECT),
	ADD_TAG(L"small", SMALL),
	ADD_TAG(L"span", SPAN),
	ADD_TAG(L"source", SOURCE),
	ADD_TAG(L"strike", STRIKE),
	ADD_TAG(L"strong", STRONG),
	ADD_TAG(L"style", STYLE),
	ADD_TAG(L"sub", SUB),
	ADD_TAG(L"summary", SUMMARY),
	ADD_TAG(L"sup", SUP),
	ADD_TAG(L"table", TABLE),
	ADD_TAG(L"tbody", TBODY),
	ADD_TAG(L"td", TD),
	ADD_TAG(L"textarea", TEXTAREA),
	ADD_TAG(L"tfoot", TFOOT),
	ADD_TAG(L"th", TH),
	ADD_TAG(L"thead", THEAD),
	ADD_TAG(L"time", TIME),
	ADD_TAG(L"title", TITLE),
	ADD_TAG(L"tr", TR),
	ADD_TAG(L"tt", TT),
	ADD_TAG(L"u", U),
	ADD_TAG(L"ul", UL),
	ADD_TAG(L"var", VAR),
	ADD_TAG(L"video", VIDEO),
	ADD_TAG(L"wbr", WBR),
	ADD_TAG(L"xmp", XMP),

	ADD_TAG(L"svg", SVG)
};

bool HTML2XHTML(const std::wstring& wsFileName, XmlUtils::CXmlLiteReader& oLiteReader)
{
	BYTE* pData;
	DWORD nLength;
	if (!NSFile::CFileBinary::ReadAllBytes(wsFileName, &pData, nLength))
		return false;

	std::string sFileContent = XmlUtils::GetUtf8FromFileContent(pData, nLength);

	bool bNeedConvert = true;
	if (nLength > 4)
	{
		if (pData[0] == 0xFF && pData[1] == 0xFE && !(pData[2] == 0x00 && pData[3] == 0x00))
			bNeedConvert = false;
		if (pData[0] == 0xFE && pData[1] == 0xFF)
			bNeedConvert = false;

		if (pData[0] == 0xFF && pData[1] == 0xFE && pData[2] == 0x00 && pData[3] == 0x00)
			bNeedConvert = false;
		if (pData[0] == 0 && pData[1] == 0 && pData[2] == 0xFE && pData[3] == 0xFF)
			bNeedConvert = false;
	}

	RELEASEARRAYOBJECTS(pData);

	size_t nFind = sFileContent.find("version=\"");
	if(nFind != std::string::npos)
	{
		nFind += 9;
		size_t nFindEnd = sFileContent.find("\"", nFind);
		if(nFindEnd != std::string::npos)
			sFileContent.replace(nFind, nFindEnd - nFind, "1.0");
	}

	const std::wstring sRes{htmlToXhtml(sFileContent, bNeedConvert)};

	#ifdef SAVE_NORMALIZED_HTML
	#if 1 == SAVE_NORMALIZED_HTML
	NSFile::CFileBinary oWriter;
	if (oWriter.CreateFileW(L"res.html"))
	{
		oWriter.WriteStringUTF8(sRes);
		oWriter.CloseFile();
	}
	#endif
	#endif

	return oLiteReader.FromString(sRes);
}

bool MHT2XHTML(const std::wstring& wsFileName, XmlUtils::CXmlLiteReader& oLiteReader)
{
	NSFile::CFileBinary file;
	if (!file.OpenFile(wsFileName))
		return false;

	unsigned char* buffer = new unsigned char[4096];
	if (!buffer)
	{
		file.CloseFile();
		return false;
	}

	DWORD dwReadBytes = 0;
	file.ReadFile(buffer, 4096, dwReadBytes);
	file.CloseFile();
	std::string xml_string = XmlUtils::GetUtf8FromFileContent(buffer, dwReadBytes);

	const std::string sContentType = NSStringFinder::FindProperty(xml_string, "content-type", ":", ";");
	bool bRes = false;

	if(NSStringFinder::Equals(sContentType, "multipart/related"))
	{
		BYTE* pData;
		DWORD nLength;
		if (!NSFile::CFileBinary::ReadAllBytes(wsFileName, &pData, nLength))
			return false;

		std::string sFileContent = XmlUtils::GetUtf8FromFileContent(pData, nLength);
		RELEASEARRAYOBJECTS(pData);
		const std::wstring sRes = mhtToXhtml(sFileContent);
		bRes = oLiteReader.FromString(sRes);
	}
	else
		bRes = HTML2XHTML(wsFileName, oLiteReader);

	RELEASEARRAYOBJECTS(buffer);
	return bRes;
}

inline std::wstring GetArgumentValue(XmlUtils::CXmlLiteReader& oLiteReader, const std::wstring& wsArgumentName, const std::wstring& wsDefaultValue = L"");
inline bool CheckArgumentMath(const std::wstring& wsNodeName, const std::wstring& wsStyleName);
inline HtmlTag GetHtmlTag(const std::wstring& wsStrTag);
inline bool UnreadableNode(const std::wstring& wsNodeName);
inline bool TagIsUnprocessed(const std::wstring& wsTagName);
inline void GetSubClass(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, NSCSS::CCssCalculator& oCSSCalculator);

CHTMLReader::CHTMLReader()
	: m_bIsTempDirOwner(true), m_pWriter(nullptr)
{}

CHTMLReader::~CHTMLReader()
{
	if (nullptr != m_pWriter)
		delete m_pWriter;

	if (m_bIsTempDirOwner && !m_wsTempDirectory.empty())
		NSDirectory::DeleteDirectory(m_wsTempDirectory);
}

void CHTMLReader::SetTempDirectory(const std::wstring& wsPath)
{
	m_wsTempDirectory = wsPath;
	m_bIsTempDirOwner = m_wsTempDirectory.empty();
}

void CHTMLReader::SetCoreDirectory(const std::wstring& wsPath)
{
	m_wsCoreDirectory = wsPath;
}

HRESULT CHTMLReader::ConvertHTML2OOXML(const std::wstring& wsPath, const std::wstring& wsDirectory, THTMLParameters* pParameters)
{
	return InitAndConvert2OOXML({wsPath}, wsDirectory, HTML2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertHTML2Markdown(const std::wstring& wsPath, const std::wstring& wsFinalFile, TMarkdownParameters* pParameters)
{
	return InitAndConvert2Markdown({wsPath}, wsFinalFile, HTML2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertHTML2OOXML(const std::vector<std::wstring>& arPaths, const std::wstring& wsDirectory, THTMLParameters* pParameters)
{
	return InitAndConvert2OOXML(arPaths, wsDirectory, HTML2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertHTML2Markdown(const std::vector<std::wstring>& arPaths, const std::wstring& wsFinalFile, TMarkdownParameters* pParameters)
{
	return InitAndConvert2Markdown(arPaths, wsFinalFile, HTML2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertMHT2OOXML(const std::wstring& wsPath, const std::wstring& wsDirectory, THTMLParameters* pParameters)
{
	return InitAndConvert2OOXML({wsPath}, wsDirectory, MHT2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertMHT2Markdown(const std::wstring& wsPath, const std::wstring& wsFinalFile, TMarkdownParameters* pParameters)
{
	return InitAndConvert2Markdown({wsPath}, wsFinalFile, MHT2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertMHT2OOXML(const std::vector<std::wstring>& arPaths, const std::wstring& wsDirectory, THTMLParameters* pParameters)
{
	return InitAndConvert2OOXML(arPaths, wsDirectory, MHT2XHTML, pParameters);
}

HRESULT CHTMLReader::ConvertMHT2Markdown(const std::vector<std::wstring>& arPaths, const std::wstring& wsFinalFile, TMarkdownParameters* pParameters)
{
	return InitAndConvert2Markdown(arPaths, wsFinalFile, MHT2XHTML, pParameters);
}

void CHTMLReader::Clear()
{
	if (nullptr != m_pWriter)
		delete m_pWriter;

	m_oTags.Clear();

	m_wsSrcDirectory .clear();
	m_wsDstDirectory .clear();
	m_wsBaseDirectory.clear();
	m_wsCoreDirectory.clear();
}

bool CHTMLReader::InitOOXMLTags(THTMLParameters* pParametrs)
{
	Clear();

	COOXMLWriter *pWriter = new COOXMLWriter(pParametrs, &m_oCSSCalculator);

	if (nullptr == pWriter)
		return false;

	if (nullptr != m_pWriter)
		delete m_pWriter;

	pWriter->SetSrcDirectory (m_wsSrcDirectory);
	pWriter->SetDstDirectory (m_wsDstDirectory);
	pWriter->SetTempDirectory(m_wsTempDirectory);
	pWriter->SetBaseDirectory(m_wsBaseDirectory);
	pWriter->SetCoreDirectory(m_wsCoreDirectory);

	m_oTags.Clear();

	if (!m_oTags.Init(pWriter))
	{
		delete pWriter;
		return false;
	}

	m_pWriter = pWriter;

	return true;
}

bool CHTMLReader::InitMDTags(TMarkdownParameters* pParametrs)
{
	CMDWriter *pWriter = new CMDWriter((nullptr != pParametrs) ? *pParametrs : TMarkdownParameters{});

	if (nullptr == pWriter)
		return false;

	if (nullptr != m_pWriter)
		delete m_pWriter;

	m_oTags.Clear();

	if (!m_oTags.Init(pWriter))
	{
		delete pWriter;
		return false;
	}

	m_pWriter = pWriter;

	return true;
}

bool CHTMLReader::IsHTML(XmlUtils::CXmlLiteReader& oReader)
{
	return ((oReader.MoveToStart() && oReader.ReadNextNode()) ? oReader.GetName() == L"html" : false);
}

HRESULT CHTMLReader::InitAndConvert2OOXML(const std::vector<std::wstring>& arPaths, const std::wstring& wsDirectory, Convert_Func Convertation, THTMLParameters* pParameters)
{
	if (!InitOOXMLTags(pParameters))
		return S_FALSE;

	m_wsDstDirectory = wsDirectory;

	HRESULT lResult{S_FALSE};

	m_pWriter->Begin(wsDirectory);

	for (const std::wstring& wsPath : arPaths)
	{
		if (Convert(wsPath, Convertation))
		{
			lResult = S_OK;

			if (nullptr != pParameters && pParameters->m_bNeedPageBreakBefore)
				m_pWriter->PageBreak();
		}
	}

	m_pWriter->End(wsDirectory);

	return lResult;
}

HRESULT CHTMLReader::InitAndConvert2Markdown(const std::vector<std::wstring>& arPaths, const std::wstring& wsFinalFile, Convert_Func Convertation, TMarkdownParameters* pParameters)
{
	if (!InitMDTags(pParameters))
		return S_FALSE;

	HRESULT lResult{S_FALSE};

	m_pWriter->Begin(L"");

	for (const std::wstring& wsPath : arPaths)
	{
		if (Convert(wsPath, Convertation))
			lResult = S_OK;
	}

	m_pWriter->End(wsFinalFile);

	return lResult;
}

bool CHTMLReader::Convert(const std::wstring& wsPath, Convert_Func Convertation)
{
	XmlUtils::CXmlLiteReader oReader;

	if (nullptr == m_pWriter || !Convertation(wsPath, oReader) || !oReader.IsValid() || !IsHTML(oReader))
		return false;

	if (m_wsTempDirectory.empty())
	{
		m_wsTempDirectory = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());
	}

	m_wsSrcDirectory = NSSystemPath::GetDirectoryName(wsPath);

	oReader.MoveToStart();
	oReader.ReadNextNode();
	ReadStyle(oReader);

	// Переходим в начало
	if(!oReader.MoveToStart())
		return S_FALSE;

	ReadDocument(oReader);

	return true;
}

void CHTMLReader::ReadStyle(XmlUtils::CXmlLiteReader& oReader)
{
	if(oReader.IsEmptyNode())
		return;

	const int nDeath = oReader.GetDepth();
	std::wstring sName;

	while(oReader.ReadNextSiblingNode(nDeath))
	{
		sName = oReader.GetName();

		if(sName == L"body")
			ReadStyle2(oReader);
		else
		{
			// Стиль по ссылке
			if(sName == L"link")
			{
				while(oReader.MoveToNextAttribute())
					ReadStyleFromNetwork(oReader);

				oReader.MoveToElement();
			}
			// тэг style содержит стили для styles.xml
			else if(sName == L"style")
				m_oCSSCalculator.AddStyles(oReader.GetText2());
			else
				ReadStyle(oReader);
		}
	}
}

void CHTMLReader::ReadStyle2(XmlUtils::CXmlLiteReader& oReader)
{
	const std::wstring wsName = oReader.GetName();
	// Стиль по ссылке
	if(wsName == L"link")
	{
		while(oReader.MoveToNextAttribute())
			ReadStyleFromNetwork(oReader);
		oReader.MoveToElement();
	}
	// тэг style содержит стили для styles.xml
	else if(wsName == L"style")
		m_oCSSCalculator.AddStyles(oReader.GetText2());

	const int nDeath = oReader.GetDepth();
	while(oReader.ReadNextSiblingNode(nDeath))
	{
		if(!oReader.IsEmptyNode())
			ReadStyle2(oReader);
	}
}

void CHTMLReader::ReadStyleFromNetwork(XmlUtils::CXmlLiteReader& oReader)
{
	if(oReader.GetName() != L"href")
		return;
	std::wstring sRef = oReader.GetText();
	if(NSFile::GetFileExtention(sRef) != L"css")
		return;
	std::wstring sFName = NSFile::GetFileName(sRef);
	// Стиль в сети
	if(sRef.substr(0, 4) == L"http")
	{
		sFName = m_wsTempDirectory + L'/' + sFName;
		NSNetwork::NSFileTransport::CFileDownloader oDownloadStyle(sRef, false);
		oDownloadStyle.SetFilePath(sFName);
		if(oDownloadStyle.DownloadSync())
		{
			m_oCSSCalculator.AddStylesFromFile(sFName);
			NSFile::CFileBinary::Remove(sFName);
		}
	}
	else
	{
		m_oCSSCalculator.AddStylesFromFile(m_wsSrcDirectory + L'/' + sFName);
		m_oCSSCalculator.AddStylesFromFile(m_wsSrcDirectory + L'/' + sRef);
	}
}

void CHTMLReader::ReadDocument(XmlUtils::CXmlLiteReader& oReader)
{
	oReader.ReadNextNode();

	int nDeath = oReader.GetDepth();
	while(oReader.ReadNextSiblingNode(nDeath))
	{
		const std::wstring wsName = oReader.GetName();
		if(wsName == L"head")
			ReadHead(oReader);
		else if(wsName == L"body")
			ReadBody(oReader);
	}
}

void CHTMLReader::ReadHead(XmlUtils::CXmlLiteReader& oReader)
{
	if(oReader.IsEmptyNode())
		return;
	int nDeath = oReader.GetDepth();
	while (oReader.ReadNextSiblingNode(nDeath))
	{
		const std::wstring wsName = oReader.GetName();
		// Базовый адрес
		if (L"base" == wsName)
			m_wsBaseDirectory = GetArgumentValue(oReader, L"href");
	}

	oReader.MoveToElement();
}

void CHTMLReader::ReadBody(XmlUtils::CXmlLiteReader& oReader)
{
	std::vector<NSCSS::CNode> arSelectors;

	arSelectors.push_back(NSCSS::CNode(L"html", L"", L""));

	GetSubClass(oReader, arSelectors, m_oCSSCalculator);

	if (!m_oTags.m_pHTML->Apply(arSelectors.back()))
		return;

	std::map<std::wstring, std::wstring>::iterator itFound = arSelectors.back().m_mAttributes.find(L"bgcolor");

	if (arSelectors.back().m_mAttributes.end() != itFound)
		arSelectors.back().m_mAttributes.erase(itFound);

	oReader.MoveToElement();

	ReadStream(oReader, arSelectors);
}

bool CHTMLReader::ReadStream(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, bool bInsertEmptyP)
{
	if (nullptr == m_pWriter)
		return false;

	bool bResult{false};

	const int nDeath = oReader.GetDepth();
	if(oReader.IsEmptyNode() || !oReader.ReadNextSiblingNode2(nDeath))
	{
		if (!bInsertEmptyP)
			return false;

		m_pWriter->WriteEmptyParagraph();
		return true;
	}

	do
	{
		if (ReadInside(oReader, arSelectors))
			bResult = true;
	} while(oReader.ReadNextSiblingNode2(nDeath));

	if (!bResult && bInsertEmptyP)
		m_pWriter->WriteEmptyParagraph();

	return bResult;
}

bool CHTMLReader::ReadInside(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors)
{
	const std::wstring wsName{oReader.GetName()};

	if(wsName == L"#text")
		return ReadText(oReader, arSelectors);

	//TODO:: обработать все варианты return'а
	if (UnreadableNode(wsName) || TagIsUnprocessed(wsName))
		return false;

	GetSubClass(oReader, arSelectors, m_oCSSCalculator);

	bool bResult{false};

	const HtmlTag eHtmlTag{GetHtmlTag(wsName)};

	switch(eHtmlTag)
	{
		case HTML_TAG(A):
		case HTML_TAG(AREA):
		{
			if (m_oTags.m_pAnchor->Open(arSelectors))
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pAnchor->Close(arSelectors.back());
			}
			break;
		}
		case HTML_TAG(ABBR):
		{
			if (m_oTags.m_pAbbr->Open(arSelectors))
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pAbbr->Close();
			}
			break;
		}
		case HTML_TAG(B):
		case HTML_TAG(STRONG):
		{
			if (m_oTags.m_pBold->Open())
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pBold->Close();
			}
			break;
		}
		case HTML_TAG(BR):
		{
			bResult = m_oTags.m_pBreak->Read(arSelectors.back());
			break;
		}
		case HTML_TAG(CITE):
		case HTML_TAG(DFN):
		case HTML_TAG(EM):
		case HTML_TAG(I):
		case HTML_TAG(VAR):
		{
			if (m_oTags.m_pItalic->Open())
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pItalic->Close();
			}
			break;
		}
		case HTML_TAG(CODE):
		case HTML_TAG(SAMP):
		case HTML_TAG(TT):
		case HTML_TAG(OUTPUT):
		case HTML_TAG(KBD):
		{
			if (m_oTags.m_pCode->Open(arSelectors.back()))
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pCode->Close();
			}
			break;
		}
		case HTML_TAG(DEL):
		case HTML_TAG(S):
		case HTML_TAG(STRIKE):
		{
			if (m_oTags.m_pStrike->Open())
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pStrike->Close();
			}
			break;
		}
		case HTML_TAG(FONT):
		{
			m_oTags.m_pFont->Apply(arSelectors.back(), arSelectors.size());
			bResult = ReadStream(oReader, arSelectors);
			break;
		}
		case HTML_TAG(IMG):
		{
			bResult = m_oTags.m_pImage->Read(arSelectors);
			break;
		}
		case HTML_TAG(SVG):
		{
			bResult = m_oTags.m_pImage->ReadSVG(arSelectors, oReader.GetOuterXml());
			break;
		}
		case HTML_TAG(INS):
		case HTML_TAG(U):
		{
			if (m_oTags.m_pUnderline->Open())
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pUnderline->Close();
			}
			break;
		}
		case HTML_TAG(INPUT):
		{
			m_oTags.m_pInput->Read(arSelectors);
			bResult = ReadStream(oReader, arSelectors);
			break;
		}
		case HTML_TAG(NOBR):
		{
			if (m_oTags.m_pPreformatted->Open())
			{
				bResult = ReadStream(oReader, arSelectors);
				m_oTags.m_pPreformatted->Close(arSelectors);
			}
			break;
		}
		case HTML_TAG(BASEFONT):
		{
			m_oTags.m_pBaseFont->Apply(arSelectors.back());
			bResult = ReadStream(oReader, arSelectors);
			break;
		}
		case HTML_TAG(BUTTON):
		case HTML_TAG(LABEL):
		case HTML_TAG(DATA):
		case HTML_TAG(OBJECT):
		case HTML_TAG(NOSCRIPT):
		case HTML_TAG(TIME):
		case HTML_TAG(SMALL):
		case HTML_TAG(PROGRESS):
		case HTML_TAG(HGROUP):
		case HTML_TAG(METER):
		case HTML_TAG(ACRONYM):
		case HTML_TAG(BIG):

		case HTML_TAG(BDO):
		case HTML_TAG(BDI):
		case HTML_TAG(CENTER):
		case HTML_TAG(MARK):
		case HTML_TAG(Q):
		case HTML_TAG(SUP):
		case HTML_TAG(SUB):
		case HTML_TAG(SPAN):
		{
			bResult = ReadStream(oReader, arSelectors);
			break;
		}

		case HTML_TAG(CANVAS):
		case HTML_TAG(VIDEO):
		case HTML_TAG(MATH):
		case HTML_TAG(IFRAME):
		case HTML_TAG(EMBED):
		case HTML_TAG(WBR):
		case HTML_TAG(AUDIO):
		case HTML_TAG(BGSOUND):
		case HTML_TAG(APPLET):
		case HTML_TAG(BLINK):
		case HTML_TAG(KEYGEN):
		case HTML_TAG(TITLE):
		case HTML_TAG(STYLE):
		case HTML_TAG(SCRIPT):
		{
			//Если встретили не обрабатываемые теги, то просто пропускаем
			arSelectors.pop_back();
			return false;
		}
		default:
		{
			m_pWriter->BeginBlock();

			switch(eHtmlTag)
			{
				case HTML_TAG(ADDRESS):
				{
					if (m_oTags.m_pItalic->Open())
					{
						bResult = ReadStream(oReader, arSelectors);
						m_oTags.m_pItalic->Close();
					}
					break;
				}
				case HTML_TAG(H1):
				case HTML_TAG(H2):
				case HTML_TAG(H3):
				case HTML_TAG(H4):
				case HTML_TAG(H5):
				case HTML_TAG(H6):
				{
					m_oTags.m_pHeader->Read(arSelectors.back());
					bResult = ReadStream(oReader, arSelectors);
					break;
				}
				case HTML_TAG(ASIDE):
				case HTML_TAG(DIV):
				{
					if (m_oTags.m_pDivision->Open(arSelectors))
					{
						bResult = ReadStream(oReader, arSelectors);
						m_oTags.m_pDivision->Close();
					}
					break;
				}
				case HTML_TAG(BLOCKQUOTE):
				{
					if (m_oTags.m_pBlockquote->Open(arSelectors))
					{
						bResult = ReadStream(oReader, arSelectors);
						m_oTags.m_pBlockquote->Close();
					}

					break;
				}
				case HTML_TAG(HR):
				{
					bResult = true;
					m_oTags.m_pHorizontalRule->Write(arSelectors);
					break;
				}
				case HTML_TAG(LI):
				{
					if (m_oTags.m_pListElement->Open())
					{
						bResult = ReadStream(oReader, arSelectors);
						m_oTags.m_pListElement->Close();
					}
					break;
				}
				case HTML_TAG(OL):
				case HTML_TAG(UL):
				{
					if (m_oTags.m_pList->Open(arSelectors.back()))
					{
						bResult = ReadStream(oReader, arSelectors);
						m_oTags.m_pList->Close();
					}
					break;
				}
		// 		case HTML_TAG(MENU):
		// 		case HTML_TAG(SELECT):
		// 		case HTML_TAG(DATALIST):
		// 		case HTML_TAG(DIR):
		// 		{
		// 			bResult = readLi(&oXmlData, sSelectors, oTS, HTML_TAG(OL) != eHtmlTag);
		// 			break;
		// 		}
				case HTML_TAG(PRE):
				case HTML_TAG(XMP):
				{
					if (m_oTags.m_pPreformatted->Open())
					{
						bResult = ReadStream(oReader, arSelectors);
						m_oTags.m_pPreformatted->Close(arSelectors);
					}
					break;
				}
				case HTML_TAG(TABLE):
				{
					bResult = ReadTable(oReader, arSelectors);
					break;
				}
				case HTML_TAG(ARTICLE):
				case HTML_TAG(HEADER):
				case HTML_TAG(MAIN):
				case HTML_TAG(SUMMARY):
				case HTML_TAG(FOOTER):
				case HTML_TAG(NAV):
				case HTML_TAG(FIGCAPTION):
				case HTML_TAG(FORM):
				case HTML_TAG(OPTION):
				case HTML_TAG(DT):
				case HTML_TAG(P):
				case HTML_TAG(SECTION):
				case HTML_TAG(FIGURE):
				case HTML_TAG(DL):
				case HTML_TAG(LEGEND):
				case HTML_TAG(MAP):
				case HTML_TAG(DD):
				default:
				{
					bResult = ReadStream(oReader, arSelectors);
					break;
				}
		// 		case HTML_TAG(RUBY):
		// 		{
		// 			bResult = ParseRuby(&oXmlData, sSelectors, oTS);
		// 			break;
		// 		}
				// case HTML_TAG(TEXTAREA):
				// case HTML_TAG(FIELDSET):
				// {
				// 	bResult = ReadStream(arSelectors);
				// 	break;
				// }
		// 		case HTML_TAG(DETAILS):
		// 		{
		// 			bResult = ReadDetails(&oXmlData, sSelectors, oTS);
		// 			break;
		// 		}
			}

			m_pWriter->EndBlock(bResult);
		}
	}

	arSelectors.pop_back();
	return bResult;
}

bool CHTMLReader::ReadText(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors)
{
	if (nullptr == m_pWriter)
		return false;

	GetSubClass(oReader, arSelectors, m_oCSSCalculator);

	const bool bResult{m_pWriter->WriteText(oReader.GetText(), arSelectors)};

	arSelectors.pop_back();

	return bResult;
}

bool CHTMLReader::ReadTable(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors)
{
	if(oReader.IsEmptyNode())
		return false;

	XmlUtils::CXmlLiteReader oTableReader;

	if (!oTableReader.FromString(oReader.GetOuterXml()) || !oTableReader.ReadNextNode())
		return false;

	// TODO:: Возможно стоит сделать не CTableElement, а CTableConverter
	// и перенести объявление в Init метод, чтобы не запрашивать тип райтера
	// А после конвертация таблицы его очищать
	CTableElement* pTableElement{nullptr};
	TExternalTableData oExternalData;

	oExternalData.ReadStream = [this](XmlUtils::CXmlLiteReader& oReader,
	                                  std::vector<NSCSS::CNode>& arSelectors)
	                                 { return ReadStream(oReader, arSelectors, true); };

	oExternalData.GetSubClass = [this](XmlUtils::CXmlLiteReader& oReader,
	                                   std::vector<NSCSS::CNode>& arSelectors)
	                                  { GetSubClass(oReader, arSelectors, m_oCSSCalculator); };

	oExternalData.m_pWriter = m_pWriter;

	switch (m_pWriter->GetType())
	{
		case EWriterType::Markdown: pTableElement = new CMarkdownTable(&oExternalData); break;
		case EWriterType::OOXML: pTableElement = new COOXMLTable(&oExternalData); break;
		default:
			return false;
	}

	if (nullptr == pTableElement)
		return false;

	if (!pTableElement->PreParse(oTableReader) ||
	    !oTableReader.MoveToStart() || !oTableReader.ReadNextNode())
	{
		delete pTableElement;
		return false;
	}

	pTableElement->Normalize();
	pTableElement->Convert(oTableReader, arSelectors.back());

	delete pTableElement;
	return true;
}

inline std::wstring GetArgumentValue(XmlUtils::CXmlLiteReader& oLiteReader, const std::wstring& wsArgumentName, const std::wstring& wsDefaultValue)
{
	if (!oLiteReader.MoveToFirstAttribute())
		return wsDefaultValue;

	std::wstring wsValue{wsDefaultValue};

	do
	{
		if (wsArgumentName == oLiteReader.GetName())
		{
			wsValue = oLiteReader.GetText();
			break;
		}
	} while (oLiteReader.MoveToNextAttribute());

	oLiteReader.MoveToElement();
	return wsValue;
}

// Так как CSS калькулятор не знает для какой ноды производится расчет стиля
// и не знает, что некоторые стили предназначены только определенной ноде,
// то проще пока обрабатывать это заранее
// ! Используется для стилей, заданных через аргументы !
inline bool CheckArgumentMath(const std::wstring& wsNodeName, const std::wstring& wsStyleName)
{
	if (L"border" == wsStyleName && L"table" != wsNodeName)
		return false;

	return true;
}

inline HtmlTag GetHtmlTag(const std::wstring& wsStrTag)
{
	std::map<std::wstring, HtmlTag>::const_iterator oFound = m_HTML_TAGS.find(wsStrTag);

	if (oFound == m_HTML_TAGS.cend())
	{
		if (wsStrTag.length() > 3 && wsStrTag.compare(wsStrTag.length() - 3, 3, L"svg") == 0)
			return HTML_TAG(SVG);
		return UNKNOWN_TAG;
	}

	return oFound->second;
}

inline bool UnreadableNode(const std::wstring& wsNodeName)
{
	return L"head" == wsNodeName || L"meta" == wsNodeName || L"style" == wsNodeName;
}

inline bool TagIsUnprocessed(const std::wstring& wsTagName)
{
	return L"xml" == wsTagName;
}

inline void GetSubClass(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, NSCSS::CCssCalculator& oCSSCalculator)
{
	NSCSS::CNode oNode;

	oNode.m_wsName = oReader.GetName();
	// Стиль по атрибуту
	std::wstring wsAttributeName;

	if (oReader.MoveToFirstAttribute())
	{
		do
		{
			wsAttributeName = oReader.GetName();
			if(wsAttributeName == L"class")
				oNode.m_wsClass  = EncodeXmlString(oReader.GetText());
			else if(wsAttributeName == L"id")
			{
				oNode.m_wsId = EncodeXmlString(oReader.GetText());
				// WriteEmptyBookmark(oXml, oNode.m_wsId);

				// if (!m_oStylesCalculator.HaveStylesById(oNode.m_wsId))
					// oNode.m_wsId.clear();
			}
			else if(wsAttributeName == L"style")
				oNode.m_wsStyle += oReader.GetText();
			else
			{
				if (CheckArgumentMath(oNode.m_wsName, wsAttributeName))
					oNode.m_mAttributes[wsAttributeName] = oReader.GetText();
			}
		}while(oReader.MoveToNextAttribute());
	}

	oReader.MoveToElement();
	arSelectors.push_back(oNode);

	oCSSCalculator.CalculateCompiledStyle(arSelectors);
}
}
