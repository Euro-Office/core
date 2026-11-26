
#pragma once

#include <string>

class CJBig2File 
{
public:
	CJBig2File();
	
	bool MemoryToJBig2(unsigned char* pBufferBGRA ,int BufferSize, int nWidth, int nHeight, std::wstring sDstFileName, bool isBGRA = true);

	bool m_bDuplicateLineRemoval;
	bool m_bPDFMode;
	bool m_bUpscale2x;
	bool m_bUpscale4x;
	int m_nBwTreshold;
};
