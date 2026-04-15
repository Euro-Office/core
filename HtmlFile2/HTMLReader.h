#ifndef HTMLREADER_H
#define HTMLREADER_H

#include "../Common/3dParty/html/css/src/CCssCalculator.h"
#include "../DesktopEditor/xml/include/xmlutils.h"

#include "HTMLParameters.h"
#include "MarkdownParameters.h"

#include "Writers/IWriter.h"
#include "Tags/HTMLTags.h"
#include "Table.h"

#include <set>

namespace HTML
{
class CHTMLReader
{
	// XmlUtils::CXmlLiteReader m_oLightReader;   // SAX Reader
	NSCSS::CCssCalculator    m_oCSSCalculator; // Css калькулятор

	bool m_bIsTempDirOwner;
	std::wstring m_wsTempDirectory; // Temp папка
	std::wstring m_wsSrcDirectory;  // Директория источника
	std::wstring m_wsDstDirectory;  // Директория назначения
	std::wstring m_wsBaseDirectory; // Полный базовый адрес
	std::wstring m_wsCoreDirectory; // Путь до корневого файла (используется для работы с Epub)

	IWriter *m_pWriter;

	CTableElement* m_pTableElement; // Table Converter

	std::set<std::wstring> m_arStopTags;

	THTMLTags m_oTags;
public:
	CHTMLReader();
	~CHTMLReader();

	void SetTempDirectory(const std::wstring& wsPath);
	void SetCoreDirectory(const std::wstring& wsPath);

	HRESULT ConvertHTML2OOXML   (const std::wstring& wsPath, const std::wstring& wsDirectory, THTMLParameters*     pParameters = nullptr);
	HRESULT ConvertHTML2Markdown(const std::wstring& wsPath, const std::wstring& wsFinalFile, TMarkdownParameters* pParameters = nullptr);

	HRESULT ConvertHTML2OOXML   (const std::vector<std::wstring>& arPaths, const std::wstring& wsDirectory, THTMLParameters*     pParameters = nullptr);
	HRESULT ConvertHTML2Markdown(const std::vector<std::wstring>& arPaths, const std::wstring& wsFinalFile, TMarkdownParameters* pParameters = nullptr);

	HRESULT ConvertMHT2OOXML    (const std::wstring& wsPath,  const std::wstring& wsDirectory,  THTMLParameters*     pParameters = nullptr);
	HRESULT ConvertMHT2Markdown (const std::wstring& wsPath,  const std::wstring& wsFinalFile,  TMarkdownParameters* pParameters = nullptr);

	HRESULT ConvertMHT2OOXML    (const std::vector<std::wstring>& arPaths,  const std::wstring& wsDirectory,  THTMLParameters*     pParameters = nullptr);
	HRESULT ConvertMHT2Markdown (const std::vector<std::wstring>& arPaths,  const std::wstring& wsFinalFile,  TMarkdownParameters* pParameters = nullptr);

	NSCSS::CCssCalculator* GetCSSCalculator();
private:
	void Clear();
	bool InitOOXMLTags(THTMLParameters* pParametrs = nullptr);
	bool InitMDTags(TMarkdownParameters* pParametrs = nullptr);

	bool IsHTML(XmlUtils::CXmlLiteReader& oReader);

	typedef std::function<bool(const std::wstring&, XmlUtils::CXmlLiteReader&)> Convert_Func;

	HRESULT InitAndConvert2OOXML(const std::vector<std::wstring>& arPaths, const std::wstring& wsDirectory, Convert_Func Convertation, THTMLParameters* pParameters = nullptr);
	HRESULT InitAndConvert2Markdown(const std::vector<std::wstring>& arPaths, const std::wstring& wsFinalFile, Convert_Func Convertation, TMarkdownParameters* pParameters = nullptr);

	bool Convert(const std::wstring& wsPath, Convert_Func Convertation);

	void ReadStyle(XmlUtils::CXmlLiteReader& oReader);
	void ReadStyle2(XmlUtils::CXmlLiteReader& oReader);
	void ReadStyleFromNetwork(XmlUtils::CXmlLiteReader& oReader);

	void ReadDocument(XmlUtils::CXmlLiteReader& oReader);
	void ReadHead(XmlUtils::CXmlLiteReader& oReader);
	void ReadBody(XmlUtils::CXmlLiteReader& oReader);

	bool ReadStream(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, bool bInsertEmptyP = false);
	bool ReadInside(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors);

	bool ReadText(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors);

	bool ReadTable(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors);

	void AddStopTag(const std::wstring& wsTag);
	void ClearStopTags();
};
}

#endif // HTMLREADER_H
