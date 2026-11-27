/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for the alpha run-length encoding of the font list thumbnails
// (fonts_thumbnail*.png.bin), see Euro-Office/DesktopEditors#356.

#include "gtest/gtest.h"

#include "../FontThumbnailAlpha.h"

#include <algorithm>
#include <cstring>
#include <random>
#include <vector>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace
{
	std::vector<BYTE> MakeBgra(const std::vector<BYTE>& alphas)
	{
		std::vector<BYTE> bgra(4 * alphas.size(), 0x11);
		for (size_t i = 0; i < alphas.size(); ++i)
			bgra[4 * i + 3] = alphas[i];
		return bgra;
	}

	std::vector<BYTE> Encode(const std::vector<BYTE>& alphas)
	{
		std::vector<BYTE> bgra = MakeBgra(alphas);
		LONG nPixels = (LONG)alphas.size();
		std::vector<BYTE> out(NSFontThumbnail::GetEncodedAlphaMaxSize(nPixels) + 1, 0xEE);
		BYTE* pEnd = NSFontThumbnail::EncodeAlpha(bgra.data(), nPixels, out.data());
		EXPECT_LE(pEnd - out.data(), NSFontThumbnail::GetEncodedAlphaMaxSize(nPixels));
		EXPECT_EQ(0xEE, out[NSFontThumbnail::GetEncodedAlphaMaxSize(nPixels)]);
		out.resize(pEnd - out.data());
		return out;
	}

	// The loop as it was before the fix, run on a copy with one extra pixel
	// so that its read past the end stays inside the buffer.
	std::vector<BYTE> EncodeOld(const std::vector<BYTE>& alphas, BYTE padAlpha)
	{
		std::vector<BYTE> padded = alphas;
		padded.push_back(padAlpha);
		std::vector<BYTE> bgra = MakeBgra(padded);
		LONG lBinarySize = (LONG)alphas.size();

		std::vector<BYTE> out(2 * lBinarySize + 1);
		BYTE* pBinaryFormatCur = out.data();
		BYTE* pImageDataAlpha = bgra.data() + 3;
		LONG nIndexCur = 0;
		while (nIndexCur < lBinarySize)
		{
			*pBinaryFormatCur++ = *pImageDataAlpha;
			if (0 != *pImageDataAlpha)
			{
				++nIndexCur;
				pImageDataAlpha += 4;
			}
			else
			{
				LONG nIndexCur0 = nIndexCur;
				LONG nIndexCurLimit = nIndexCur0 + 255;
				if (nIndexCurLimit > lBinarySize)
					nIndexCurLimit = lBinarySize;
				while (nIndexCur < nIndexCurLimit)
				{
					++nIndexCur;
					pImageDataAlpha += 4;
					if (0 != *pImageDataAlpha)
						break;
				}
				*pBinaryFormatCur++ = (BYTE)(nIndexCur - nIndexCur0);
			}
		}
		out.resize(pBinaryFormatCur - out.data());
		return out;
	}
}

TEST(FontThumbnailAlpha, EncodesOpaquePixelsAndTransparentRuns)
{
	EXPECT_EQ((std::vector<BYTE>{255, 0, 3, 128, 0, 1}),
			  Encode({255, 0, 0, 0, 128, 0}));
}

TEST(FontThumbnailAlpha, SplitsTransparentRunsAt255)
{
	std::vector<BYTE> alphas(600, 0);
	EXPECT_EQ((std::vector<BYTE>{0, 255, 0, 255, 0, 90}), Encode(alphas));

	std::vector<BYTE> exact(255, 0);
	exact.push_back(7);
	EXPECT_EQ((std::vector<BYTE>{0, 255, 7}), Encode(exact));
}

TEST(FontThumbnailAlpha, WorstCaseFitsTheBufferSize)
{
	// opaque, transparent, opaque, ... uses 1.5 bytes per pixel
	std::vector<BYTE> alphas;
	for (int i = 0; i < 1000; ++i)
		alphas.push_back(i % 2 ? 0 : 200);
	std::vector<BYTE> out = Encode(alphas);
	EXPECT_EQ(1500u, out.size());
}

TEST(FontThumbnailAlpha, OutputMatchesTheOldEncoder)
{
	std::mt19937 rng(356);
	for (int round = 0; round < 200; ++round)
	{
		std::vector<BYTE> alphas(1 + rng() % 2000);
		// mostly transparent with some long runs, like a font name strip
		for (BYTE& a : alphas)
			a = (rng() % 4 == 0) ? (BYTE)(1 + rng() % 255) : 0;
		if (round % 3 == 0)
			std::fill(alphas.end() - std::min<size_t>(alphas.size(), 300), alphas.end(), 0);

		std::vector<BYTE> expected = EncodeOld(alphas, 0);
		EXPECT_EQ(expected, EncodeOld(alphas, 99)) << "old output depends on the byte past the end";
		EXPECT_EQ(expected, Encode(alphas)) << "round " << round;
	}
}

#if !defined(_WIN32)
// The crash of #356: the image ends right before an unmapped page and its
// last pixels are transparent. The old loop read the next pixel's alpha.
TEST(FontThumbnailAlpha, DoesNotReadPastAnImageEndingAtAPageBoundary)
{
	const long nPage = sysconf(_SC_PAGESIZE);
	ASSERT_GT(nPage, 0);

	BYTE* pMap = (BYTE*)mmap(nullptr, 2 * nPage, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	ASSERT_NE(MAP_FAILED, (void*)pMap);
	ASSERT_EQ(0, mprotect(pMap + nPage, nPage, PROT_NONE));

	for (LONG nPixels : {1L, 3L, 256L, nPage / 4})
	{
		BYTE* pImage = pMap + nPage - 4 * nPixels;
		memset(pImage, 0, 4 * nPixels);
		pImage[3] = 255; // one opaque pixel, then a transparent run to the end

		std::vector<BYTE> out(NSFontThumbnail::GetEncodedAlphaMaxSize(nPixels));
		BYTE* pEnd = NSFontThumbnail::EncodeAlpha(pImage, nPixels, out.data());

		EXPECT_EQ(255, out[0]);
		LONG nDecoded = 1;
		for (BYTE* p = out.data() + 1; p < pEnd; p += 2)
		{
			EXPECT_EQ(0, p[0]);
			nDecoded += p[1];
		}
		EXPECT_EQ(nPixels, nDecoded);
	}

	munmap(pMap, 2 * nPage);
}
#endif
