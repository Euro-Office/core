// Regression tests for the bounds checks in the Formatted Disk Page (FKP) parsers
// of the legacy Word binary (.doc) reader. Every case builds a 512 byte FKP by
// hand, stores it in a real compound file through POLE and hands the stream to
// the real constructors, the way WordDocument does.
//
// The malformed pages used to make the parser read past its 512 byte buffer.
// The assertions check the observable result of the bounds checks; building
// the suite with -fsanitize=address additionally turns any remaining
// out-of-bounds read into a hard failure.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "../FormattedDiskPageCHPX.h"
#include "../FormattedDiskPagePAPX.h"

namespace DocFileFormat
{
	// friend of the FKP classes, gives the tests read access to the parsed page
	struct FkpTestAccess
	{
		static unsigned int Crun(const FormattedDiskPage& page)
		{
			return page.crun;
		}
		static unsigned int RgfcSize(const FormattedDiskPage& page)
		{
			return page.rgfcSize;
		}
		static unsigned int ChpxCount(const FormattedDiskPageCHPX& page)
		{
			return page.grpchpxSize;
		}
		static CharacterPropertyExceptions* Chpx(const FormattedDiskPageCHPX& page, unsigned int i)
		{
			return page.grpchpx[i];
		}
		static unsigned int PapxCount(const FormattedDiskPagePAPX& page)
		{
			return page.grppapxSize;
		}
		static ParagraphPropertyExceptions* Papx(const FormattedDiskPagePAPX& page, unsigned int i)
		{
			return page.grppapx[i];
		}
	};
}

using namespace DocFileFormat;

namespace
{
	const int FKP_SIZE = 512;

	const int WORD_97 = 0;
	const int WORD_95 = 1;
	const int WORD_2 = 2;

	// one FKP page, zero filled, with crun in its last byte and an ascending
	// rgfc array so that a well formed page stays well formed
	std::vector<unsigned char> MakePage(unsigned char crun)
	{
		std::vector<unsigned char> page(FKP_SIZE, 0);
		for (int i = 0; i <= crun && (i + 1) * 4 <= FKP_SIZE - 1; ++i)
		{
			unsigned int fc = 0x400 + i * 0x10;
			memcpy(&page[i * 4], &fc, 4);
		}
		page[FKP_SIZE - 1] = crun;
		return page;
	}

	// a compound file on disk holding a single WordDocument stream
	class WordStream
	{
	public:
		explicit WordStream(const std::vector<unsigned char>& data)
			: m_storage(NULL), m_stream(NULL)
		{
			const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
			m_path = ::testing::TempDir() + "doc_fkp_" + info->test_suite_name() + "_" + info->name() + ".cfb";
			std::wstring path(m_path.begin(), m_path.end());

			{
				POLE::Storage writer(path.c_str());
				writer.open(true, true);
				POLE::Stream stream(&writer, L"WordDocument", true, data.size());
				stream.write(const_cast<unsigned char*>(data.data()), data.size());
				stream.flush();
				writer.close();
			}

			m_storage = new POLE::Storage(path.c_str());
			if (m_storage->open())
			{
				m_stream = new POLE::Stream(m_storage, L"WordDocument");
			}
		}
		~WordStream()
		{
			delete m_stream;
			delete m_storage;
			std::remove(m_path.c_str());
		}
		POLE::Stream* Get() const
		{
			return m_stream;
		}

	private:
		std::string m_path;
		POLE::Storage* m_storage;
		POLE::Stream* m_stream;
	};
}

// --- CHPX ------------------------------------------------------------------

TEST(FkpChpx, WellFormedPageParses)
{
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 240;                  // rgb[0]: CHPX at byte 480
	page[480] = 3;                  // cb
	page[481] = 0x35;               // sprmCFBold
	page[482] = 0x08;
	page[483] = 0x01;

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);
	ASSERT_EQ(stream.Get()->size(), (POLE::uint64)FKP_SIZE);

	FormattedDiskPageCHPX fkp(stream.Get(), 0, WORD_97);

	ASSERT_EQ(FkpTestAccess::Crun(fkp), 1u);
	ASSERT_EQ(FkpTestAccess::ChpxCount(fkp), 1u);
	CharacterPropertyExceptions* chpx = FkpTestAccess::Chpx(fkp, 0);
	ASSERT_TRUE(chpx != NULL);
	ASSERT_TRUE(chpx->grpprl != NULL);
	EXPECT_EQ(chpx->grpprl->size(), 1u);
}

TEST(FkpChpx, CrunIsCappedToOnePage)
{
	// 255 runs would need 255 * 5 + 4 bytes, the walk used to index ~1278
	WordStream stream(MakePage(255));
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPageCHPX fkp(stream.Get(), 0, WORD_97);

	EXPECT_EQ(FkpTestAccess::Crun(fkp), 101u);
	EXPECT_EQ(FkpTestAccess::RgfcSize(fkp), 102u);
	EXPECT_EQ(FkpTestAccess::ChpxCount(fkp), 101u);
}

TEST(FkpChpx, OversizedCbIsDropped)
{
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 200;                  // CHPX at byte 400
	page[400] = 255;                // 255 bytes from 401 run past the page
	memset(&page[401], 0x35, 110);

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPageCHPX fkp(stream.Get(), 0, WORD_97);

	ASSERT_EQ(FkpTestAccess::Crun(fkp), 1u);
	CharacterPropertyExceptions* chpx = FkpTestAccess::Chpx(fkp, 0);
	ASSERT_TRUE(chpx != NULL);
	EXPECT_TRUE(chpx->grpprl == NULL || chpx->grpprl->empty());
}

TEST(FkpChpx, ShortReadParsesNothing)
{
	// the page number points one page past the end of the stream
	WordStream stream(MakePage(1));
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPageCHPX fkp(stream.Get(), FKP_SIZE, WORD_97);

	EXPECT_EQ(FkpTestAccess::Crun(fkp), 0u);
	EXPECT_EQ(FkpTestAccess::ChpxCount(fkp), 0u);
}

TEST(FkpChpx, NegativeOffsetParsesNothing)
{
	WordStream stream(MakePage(1));
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPageCHPX fkp(stream.Get(), -FKP_SIZE, WORD_97);

	EXPECT_EQ(FkpTestAccess::Crun(fkp), 0u);
	EXPECT_EQ(FkpTestAccess::ChpxCount(fkp), 0u);
}

// --- PAPX ------------------------------------------------------------------

TEST(FkpPapx, WellFormedPageParses)
{
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 240;                  // BX[0]: PAPX at byte 480, 12 byte PHE follows
	page[480] = 3;                  // cw: 6 bytes
	page[481] = 0x05;               // istd 5
	page[482] = 0x00;
	page[483] = 0x03;               // sprmPJc80
	page[484] = 0x24;
	page[485] = 0x01;

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), 0, NULL, WORD_97, false);

	ASSERT_EQ(FkpTestAccess::Crun(fkp), 1u);
	ASSERT_EQ(FkpTestAccess::PapxCount(fkp), 1u);
	ParagraphPropertyExceptions* papx = FkpTestAccess::Papx(fkp, 0);
	ASSERT_TRUE(papx != NULL);
	EXPECT_EQ(papx->istd, 5);
}

TEST(FkpPapx, CrunIsCappedToOnePage)
{
	// a BX is 13 bytes in Word 97, 7 in Word 95 and 1 in Word 2
	WordStream stream(MakePage(255));
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX word97(stream.Get(), 0, NULL, WORD_97, false);
	EXPECT_EQ(FkpTestAccess::Crun(word97), 29u);
	EXPECT_EQ(FkpTestAccess::PapxCount(word97), 29u);

	FormattedDiskPagePAPX complex(stream.Get(), 0, NULL, WORD_95, true);
	EXPECT_EQ(FkpTestAccess::Crun(complex), 29u);

	FormattedDiskPagePAPX word95(stream.Get(), 0, NULL, WORD_95, false);
	EXPECT_EQ(FkpTestAccess::Crun(word95), 46u);

	FormattedDiskPagePAPX word2(stream.Get(), 0, NULL, WORD_2, false);
	EXPECT_EQ(FkpTestAccess::Crun(word2), 101u);
}

TEST(FkpPapx, OversizedCwIsDropped)
{
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 200;                  // PAPX at byte 400
	page[400] = 255;                // 510 bytes from 401 run past the page

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), 0, NULL, WORD_97, false);

	ASSERT_EQ(FkpTestAccess::Crun(fkp), 1u);
	EXPECT_TRUE(FkpTestAccess::Papx(fkp, 0) == NULL);
}

TEST(FkpPapx, PadByteAtPageEndIsNotReadAsCount)
{
	// a pad byte at 510 would make byte 511, which is crun, the word count
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 255;                  // PAPX at byte 510
	page[510] = 0;                  // pad byte

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), 0, NULL, WORD_97, false);

	ASSERT_EQ(FkpTestAccess::Crun(fkp), 1u);
	EXPECT_TRUE(FkpTestAccess::Papx(fkp, 0) == NULL);
}

TEST(FkpPapx, Word2HeightOutsidePageIsIgnored)
{
	// Word 2 reads the paragraph height at wordOffset * 2 + j + 1, which adds
	// two offsets from the file. 255 puts it at byte 520.
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 255;

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), 0, NULL, WORD_2, false);

	EXPECT_EQ(FkpTestAccess::Crun(fkp), 1u);
}

TEST(FkpPapx, ShortReadParsesNothing)
{
	// only part of the second page exists, and it is not to be trusted
	std::vector<unsigned char> data = MakePage(1);
	data.resize(FKP_SIZE + 300, 0xFF);

	WordStream stream(data);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), FKP_SIZE, NULL, WORD_97, false);

	EXPECT_EQ(FkpTestAccess::Crun(fkp), 0u);
	EXPECT_EQ(FkpTestAccess::PapxCount(fkp), 0u);
}

TEST(FkpPapx, NegativeOffsetParsesNothing)
{
	WordStream stream(MakePage(1));
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), -FKP_SIZE, NULL, WORD_97, false);

	EXPECT_EQ(FkpTestAccess::Crun(fkp), 0u);
	EXPECT_EQ(FkpTestAccess::PapxCount(fkp), 0u);
}

// --- GetFirstPapx ------------------------------------------------------------

TEST(FkpFirstPapx, MissingOrEmptyListGivesNull)
{
	EXPECT_TRUE(FormattedDiskPagePAPX::GetFirstPapx(NULL) == NULL);

	std::vector<FormattedDiskPagePAPX*> fkps;
	EXPECT_TRUE(FormattedDiskPagePAPX::GetFirstPapx(&fkps) == NULL);
}

TEST(FkpFirstPapx, PageWithoutRunsGivesNull)
{
	// what a skipped or short page leaves behind
	WordStream stream(MakePage(1));
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), FKP_SIZE, NULL, WORD_97, false);
	ASSERT_EQ(FkpTestAccess::PapxCount(fkp), 0u);

	std::vector<FormattedDiskPagePAPX*> fkps(1, &fkp);
	EXPECT_TRUE(FormattedDiskPagePAPX::GetFirstPapx(&fkps) == NULL);
}

TEST(FkpFirstPapx, WellFormedPageGivesFirstPapx)
{
	std::vector<unsigned char> page = MakePage(1);
	page[8] = 240;
	page[480] = 1;                  // cw: 2 bytes, istd only
	page[481] = 0x07;

	WordStream stream(page);
	ASSERT_TRUE(stream.Get() != NULL);

	FormattedDiskPagePAPX fkp(stream.Get(), 0, NULL, WORD_97, false);

	std::vector<FormattedDiskPagePAPX*> fkps(1, &fkp);
	ParagraphPropertyExceptions* papx = FormattedDiskPagePAPX::GetFirstPapx(&fkps);
	ASSERT_TRUE(papx != NULL);
	EXPECT_EQ(papx, FkpTestAccess::Papx(fkp, 0));
	EXPECT_EQ(papx->istd, 7);
}
