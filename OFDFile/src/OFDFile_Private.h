#ifndef OFDFILE_PRIVATE_H
#define OFDFILE_PRIVATE_H

#include "Base.h"

namespace NSFonts { class IFontManager; }

class COFDFile_Private
{
	IFolder*           m_pFolder;

	OFD::CFontChecker* m_pFontChecker;

	std::wstring       m_wsTempDir;
	bool               m_bIsTempDirOwner;

	OFD::CBase         m_oBase;

	bool Read();
public:
	COFDFile_Private(NSFonts::IApplicationFonts* pFonts);
	~COFDFile_Private();

	void Close();

	void SetTempDir(const std::wstring& wsPath);
	std::wstring GetTempDir() const;

	bool LoadFromFile(const std::wstring& wsFilePath);
	bool LoadFromMemory(BYTE* pData, DWORD ulLength);

	unsigned int GetPageCount() const;
	void GetPageSize(int nPageIndex, double& dWidth, double& dHeight) const;

	void DrawPage(IRenderer* pRenderer, int nPageIndex);
	void DrawPage(IRenderer* pRenderer, int nPageIndex, const double& dX, const double& dY, const double& dWidth, const double& dHeight);

	NSFonts::IApplicationFonts* GetFonts();
};

#endif // OFDFILE_PRIVATE_H
