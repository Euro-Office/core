#ifndef IWRITER_H
#define IWRITER_H

#include "../../DesktopEditor/graphics/pro/Fonts.h"

#include "../../Common/3dParty/html/css/src/CNode.h"
#include "../Common.h"

namespace HTML
{
enum class EWriterType
{
	OOXML,
	Markdown
};

class IWriter
{
protected:
	const std::wstring *m_pTempDir;  // Temp папка
	const std::wstring *m_pSrcPath;  // Директория источника
	const std::wstring *m_pBasePath; // Полный базовый адрес
	const std::wstring *m_pCorePath; // Путь до корневого файла (используется для работы с Epub)

	NSFonts::IApplicationFonts* m_pFonts;     // Необходимо для оптимизации работы со шрифтами
public:
	IWriter()
		: m_pTempDir(nullptr), m_pSrcPath(nullptr),
		  m_pBasePath(nullptr), m_pCorePath(nullptr),
		  m_pFonts(nullptr)
	{};

	virtual ~IWriter()
	{
		if (nullptr != m_pFonts)
			delete m_pFonts;
	};

	virtual void Begin(const std::wstring& wsDst) = 0;
	virtual void End(const std::wstring& wsDst) = 0;

	virtual bool WriteText(std::wstring wsText, const std::vector<NSCSS::CNode>& arSelectors) = 0;

	virtual void WriteEmptyParagraph(bool bVahish = false, bool bInP = false) = 0;

	virtual void PageBreak() = 0;

	virtual void BeginBlock() = 0;
	virtual void EndBlock(bool bAddBlock) = 0;

	virtual void SetDataOutput(XmlString* pOutputData) = 0; // Задаем место вывода для интерпретатора
	virtual void RevertDataOutput() = 0; // Возвращаем место вывода к исходному

	virtual XmlString* GetCurrentDocument() const = 0;

	virtual EWriterType GetType() const = 0;

	void SetSrcDirectory (const std::wstring& wsPath)
	{
		m_pSrcPath = &wsPath;
	}
	void SetTempDirectory(const std::wstring& wsPath)
	{
		m_pTempDir = &wsPath;
	}
	void SetBaseDirectory(const std::wstring& wsPath)
	{
		m_pBasePath = &wsPath;
	}
	void SetCoreDirectory(const std::wstring& wsPath)
	{
		m_pCorePath = &wsPath;
	}

	std::wstring GetTempDir()  const
	{
		return (nullptr != m_pTempDir) ? *m_pTempDir : std::wstring();
	}
	std::wstring GetSrcPath()  const
	{
		return (nullptr != m_pSrcPath) ? *m_pSrcPath : std::wstring();
	}
	std::wstring GetBasePath() const
	{
		return (nullptr != m_pBasePath) ? *m_pBasePath : std::wstring();
	}
	std::wstring GetCorePath() const
	{
		return (nullptr != m_pCorePath) ? *m_pCorePath : std::wstring();
	}

	NSFonts::IApplicationFonts* GetFonts()
	{
		if (nullptr == m_pFonts)
		{
			m_pFonts = NSFonts::NSApplication::Create();

			if (NULL != m_pFonts)
				m_pFonts->Initialize();
		}

		return m_pFonts;
	}
};
}

#endif // IWRITER_H
