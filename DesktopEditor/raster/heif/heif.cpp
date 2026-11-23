#include "heif.h"
#include "../../common/File.h"

#include <libheif/heif.h>

#include <memory>

namespace NSHeif {
	namespace
	{
		// libheif needs at least this many bytes to recognize a file
		const DWORD c_nHeaderSize = 12;
		// Same limit as CxImage (CXIMAGE_MAX_MEMORY) for the decoded BGRA buffer
		const unsigned long long c_nMaxFrameBytes = 1073741824;

		struct CContextDeleter { void operator()(heif_context* p) const { heif_context_free(p); } };
		struct CHandleDeleter { void operator()(heif_image_handle* p) const { heif_image_handle_release(p); } };
		struct CImageDeleter { void operator()(heif_image* p) const { heif_image_release(p); } };

		bool Decode(BYTE* buffer, DWORD size, CBgraFrame* frame, bool isRGBA)
		{
			std::unique_ptr<heif_context, CContextDeleter> ctx(heif_context_alloc());
			if (!ctx || heif_context_read_from_memory_without_copy(ctx.get(), buffer, size, nullptr).code != heif_error_Ok)
				return false;

			heif_image_handle* handle_raw = nullptr;
			if (heif_context_get_primary_image_handle(ctx.get(), &handle_raw).code != heif_error_Ok)
				return false;
			std::unique_ptr<heif_image_handle, CHandleDeleter> handle(handle_raw);

			const int handle_width = heif_image_handle_get_width(handle.get());
			const int handle_height = heif_image_handle_get_height(handle.get());
			if (handle_width <= 0 || handle_height <= 0 || 4ULL * handle_width * handle_height > c_nMaxFrameBytes)
				return false;

			// 8 bits per channel whatever the bit depth of the image; libheif
			// also applies the rotation and mirroring stored in the file
			heif_image* img_raw = nullptr;
			if (heif_decode_image(handle.get(), &img_raw, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr).code != heif_error_Ok)
				return false;
			std::unique_ptr<heif_image, CImageDeleter> img(img_raw);

			const int width = heif_image_get_width(img.get(), heif_channel_interleaved);
			const int height = heif_image_get_height(img.get(), heif_channel_interleaved);
			int stride = 0;
			const BYTE* source = heif_image_get_plane_readonly(img.get(), heif_channel_interleaved, &stride);
			if (!source || width <= 0 || height <= 0 || stride < 4 * width)
				return false;

			BYTE* data = new (std::nothrow) BYTE[(size_t)4 * width * height];
			if (!data)
				return false;

			for (int y = 0; y < height; ++y)
			{
				const BYTE* src = source + (size_t)y * stride;
				BYTE* dst = data + (size_t)4 * width * y;
				for (int x = 0; x < width; ++x, src += 4, dst += 4)
				{
					dst[0] = isRGBA ? src[0] : src[2];
					dst[1] = src[1];
					dst[2] = isRGBA ? src[2] : src[0];
					dst[3] = src[3];
				}
			}

			frame->put_Width(width);
			frame->put_Height(height);
			frame->put_Stride(4 * width);
			frame->put_Data(data);
			return true;
		}
	}

	bool CHeifFile::isHeif(const std::wstring& fileName)
	{
		NSFile::CFileBinary file;
		if (!file.OpenFile(fileName))
			return false;
		BYTE header[c_nHeaderSize];
		DWORD read = 0;
		file.ReadFile(header, c_nHeaderSize, read);
		return isHeif(header, read);
	}

	bool CHeifFile::isHeif(BYTE* buffer, DWORD size)
	{
		if (!buffer || size < c_nHeaderSize)
			return false;
		return heif_check_filetype(buffer, (int)c_nHeaderSize) == heif_filetype_yes_supported;
	}

	bool CHeifFile::Open(CBgraFrame* frame, const std::wstring& fileName, bool isRGBA)
	{
		BYTE* data = nullptr;
		DWORD size = 0;
		if (!NSFile::CFileBinary::ReadAllBytes(fileName, &data, size))
			return false;
		const bool result = Decode(data, size, frame, isRGBA);
		RELEASEARRAYOBJECTS(data);
		return result;
	}

	bool CHeifFile::Open(CBgraFrame* frame, BYTE* buffer, DWORD size, bool isRGBA)
	{
		if (!buffer || size < c_nHeaderSize)
			return false;
		return Decode(buffer, size, frame, isRGBA);
	}
}
