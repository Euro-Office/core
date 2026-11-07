#ifndef _BUILD_RASTER_HEIF_H_
#define _BUILD_RASTER_HEIF_H_

#include "../BgraFrame.h"

namespace NSHeif {
	class CHeifFile {
	private:
		CHeifFile() = delete;
	public:
		// HEIF images libheif can decode: HEIC (HEVC) and AVIF (AV1)
		static bool isHeif(const std::wstring& fileName);
		static bool isHeif(BYTE* buffer, DWORD size);

		// The primary image as 8-bit RGBA (not premultiplied), rotated and
		// mirrored as the file says
		static bool Open(CBgraFrame* frame, const std::wstring& fileName, bool isRGBA);
		static bool Open(CBgraFrame* frame, BYTE* buffer, DWORD size, bool isRGBA);
	};
}

#endif // _BUILD_RASTER_HEIF_H_
