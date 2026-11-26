/*
 * TIFF file IO, using CxFile.
 */

#include <stdio.h>
#include <string.h>

#include "ximage.h"

#if CXIMAGE_SUPPORT_TIF

#include <tiffio.h>

#include "xfile.h"

static tmsize_t
_tiffReadProcEx(thandle_t fd, void* buf, tmsize_t size)
{
	tmsize_t nReadCount = (tmsize_t)((CxFile*)fd)->Read(buf, 1, (size_t)size);

	if (nReadCount < size)
	{
		memset(static_cast<char*>(buf) + nReadCount, 0, size - nReadCount);
		return size;
	}

	return nReadCount;
}

static tmsize_t
_tiffWriteProcEx(thandle_t fd, void* buf, tmsize_t size)
{
	return (tmsize_t)((CxFile*)fd)->Write(buf, 1, (size_t)size);
}

static toff_t
_tiffSeekProcEx(thandle_t fd, toff_t off, int whence)
{
	// CxFile offsets are 32 bit
	if (off > 0x7FFFFFFF)
		return (toff_t)-1;
	if (!((CxFile*)fd)->Seek((int32_t)off, whence))
		return (toff_t)-1;
	if (whence == SEEK_SET)
		return off;

	return (toff_t)((CxFile*)fd)->Tell();
}

// Return nonzero if error
static int
_tiffCloseProcEx(thandle_t /*fd*/)
{
//	return !((CxFile*)fd)->Close(); // "//" needed for memory files <DP>
	return 0;
}

static toff_t
_tiffSizeProcEx(thandle_t fd)
{
	int32_t nSize = ((CxFile*)fd)->Size();
	return nSize < 0 ? 0 : (toff_t)nSize;
}

static int
_tiffMapProcEx(thandle_t /*fd*/, void** /*pbase*/, toff_t* /*psize*/)
{
	return (0);
}

static void
_tiffUnmapProcEx(thandle_t /*fd*/, void* /*base*/, toff_t /*size*/)
{
}

// libtiff reports problems with damaged files on stderr by default. Keep them
// quiet, as the bundled libtiff did in release builds.
static int
_tiffSilentHandlerEx(TIFF* /*tif*/, void* /*user_data*/, const char* /*module*/, const char* /*fmt*/, va_list /*ap*/)
{
	return 1;
}

extern "C" TIFF* _TIFFOpenEx(CxFile* stream, const char* mode)
{
	TIFFOpenOptions* opts = TIFFOpenOptionsAlloc();
	if (!opts)
		return NULL;
	TIFFOpenOptionsSetErrorHandlerExtR(opts, _tiffSilentHandlerEx, NULL);
	TIFFOpenOptionsSetWarningHandlerExtR(opts, _tiffSilentHandlerEx, NULL);

	TIFF* tif = TIFFClientOpenExt("TIFF IMAGE", mode,
	    (thandle_t) stream,
	    _tiffReadProcEx, _tiffWriteProcEx, _tiffSeekProcEx, _tiffCloseProcEx,
	    _tiffSizeProcEx, _tiffMapProcEx, _tiffUnmapProcEx, opts);

	TIFFOpenOptionsFree(opts);
	return tif;
}

#endif
