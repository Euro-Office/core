// JBig2File.cpp : Implementation of CJBig2File


#include "JBig2File.h"

#include <jbig2enc.h>
#include <leptonica/allheaders.h>

#include "../../../common/File.h"

CJBig2File::CJBig2File()
{
	m_bDuplicateLineRemoval = false;
	m_bPDFMode				= true;
	m_bUpscale2x			= false;
	m_bUpscale4x			= false;
	m_nBwTreshold			= 188;
}

bool CJBig2File::MemoryToJBig2(unsigned char* pBufferBGRA ,int BufferSize, int nWidth, int nHeight, std::wstring sDstFileName, bool isBGRA)
{
	if ( NULL == pBufferBGRA )	return false;

	PIX *pSource = pixCreate( nWidth, nHeight, 32 );
	if ( !pSource )	return false;

	// 4 bytes per pixel, the alpha byte is ignored
	unsigned char *pSourceBuffer = pBufferBGRA;
	unsigned char ShiftR = isBGRA ? 2 : 0;
	unsigned char ShiftG = 1;
	unsigned char ShiftB = isBGRA ? 0 : 2;
	for ( int nY = 0; nY < nHeight; nY++ )
	{
		for ( int nX = 0; nX < nWidth; nX++, pSourceBuffer += 4 )
		{
			pixSetRGBPixel( pSource, nX, nY, pSourceBuffer[ ShiftR ], pSourceBuffer[ ShiftG ], pSourceBuffer[ ShiftB ] );
		}
	}

	PIX *pGray = pixConvertRGBToGrayFast( pSource );
	pixDestroy( &pSource );
	if ( !pGray )
		return false;

	PIX *pBinary = NULL;
	if ( m_bUpscale2x )
		pBinary = pixScaleGray2xLIThresh( pGray, m_nBwTreshold );
	else if ( m_bUpscale4x )
		pBinary = pixScaleGray4xLIThresh( pGray, m_nBwTreshold );
	else
		pBinary = pixThresholdToBinary( pGray, m_nBwTreshold );
	pixDestroy( &pGray );
	if ( !pBinary )
		return false;

	// Without the file header and the end-of-file segment in PDF mode
	int nLength = 0;
	uint8_t *pBuffer = jbig2_encode_generic( pBinary, !m_bPDFMode, 0, 0, m_bDuplicateLineRemoval, &nLength );
	pixDestroy( &pBinary );
	if ( !pBuffer )
		return false;

	bool bRes = false;
	NSFile::CFileBinary file;
	if ( file.CreateFileW( sDstFileName ) )
	{
		bRes = file.WriteFile( pBuffer, nLength );
		file.CloseFile();
	}
	free( pBuffer );

	return bRes;
}
