// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Regression tests for CRecordExObjStg (GHSA-83rf-4hgw-9ppp). The record takes
// its sizes from its own header and from the object it stores, and both are
// written by whoever made the file. Each case puts one record into the
// "PowerPoint Document" stream of a fresh compound file, reads it back the way
// CPPTDocumentInfo::GetBinFromStg does, and checks what the reader left in the
// temp folder.

#include "gtest/gtest.h"

#include "../Records/ExObjListContainer.h"
#include "../../../DesktopEditor/common/Directory.h"
#include "../../../DesktopEditor/common/File.h"
#include "../../../OfficeUtils/src/OfficeUtils.h"

#include <vector>

using namespace PPT;

namespace
{
    const unsigned short RT_EXTERNAL_OLE_OBJECT_STG = 0x1011;

    // RecInstance 1 marks a compressed object: the body is the uncompressed size
    // followed by a zlib stream. RecInstance 0 means the body is the object.
    const unsigned short INSTANCE_STORED = 0;
    const unsigned short INSTANCE_COMPRESSED = 1;

    void AppendUInt16(std::vector<BYTE>& arData, unsigned short nValue)
    {
        arData.push_back((BYTE)(nValue & 0xFF));
        arData.push_back((BYTE)(nValue >> 8));
    }

    void AppendUInt32(std::vector<BYTE>& arData, _UINT32 nValue)
    {
        for (int i = 0; i < 4; ++i)
            arData.push_back((BYTE)(nValue >> (8 * i)));
    }

    // The header is written as given, so RecLen can disagree with the body.
    std::vector<BYTE> MakeRecord(unsigned short nInstance, _UINT32 nRecLen, const std::vector<BYTE>& arBody)
    {
        std::vector<BYTE> arRecord;
        AppendUInt16(arRecord, (unsigned short)(nInstance << 4));
        AppendUInt16(arRecord, RT_EXTERNAL_OLE_OBJECT_STG);
        AppendUInt32(arRecord, nRecLen);
        arRecord.insert(arRecord.end(), arBody.begin(), arBody.end());
        return arRecord;
    }

    std::vector<BYTE> MakeRecord(unsigned short nInstance, const std::vector<BYTE>& arBody)
    {
        return MakeRecord(nInstance, (_UINT32)arBody.size(), arBody);
    }

    std::vector<BYTE> MakeCompressedBody(_UINT32 nClaimedSize, const std::vector<BYTE>& arCompressed)
    {
        std::vector<BYTE> arBody;
        AppendUInt32(arBody, nClaimedSize);
        arBody.insert(arBody.end(), arCompressed.begin(), arCompressed.end());
        return arBody;
    }

    std::vector<BYTE> MakeObject(size_t nSize)
    {
        std::vector<BYTE> arObject(nSize);
        for (size_t i = 0; i < nSize; ++i)
            arObject[i] = (BYTE)(i % 251);
        return arObject;
    }

    std::vector<BYTE> Compress(const std::vector<BYTE>& arData)
    {
        ULONG nSize = (ULONG)(arData.size() + arData.size() / 100 + 1024);
        std::vector<BYTE> arCompressed(nSize);

        COfficeUtils oUtils;
        if (S_OK != oUtils.Compress(arCompressed.data(), &nSize, const_cast<BYTE*>(arData.data()), (ULONG)arData.size()))
        {
            ADD_FAILURE() << "zlib could not compress the test object";
            return std::vector<BYTE>();
        }

        arCompressed.resize(nSize);
        return arCompressed;
    }

    std::vector<BYTE> ReadAll(const std::wstring& sFile)
    {
        BYTE* pData = NULL;
        DWORD nSize = 0;
        if (!NSFile::CFileBinary::ReadAllBytes(sFile, &pData, nSize))
            return std::vector<BYTE>();

        std::vector<BYTE> arData(pData, pData + nSize);
        delete[] pData;
        return arData;
    }
}

class PptExObjStgTest : public ::testing::Test
{
protected:
    std::wstring m_sDir;
    std::wstring m_sTempPath;

    void SetUp() override
    {
        m_sDir = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());
        ASSERT_TRUE(NSDirectory::Exists(m_sDir));

        m_sTempPath = m_sDir + FILE_SEPARATOR_STR + L"temp";
        ASSERT_TRUE(NSDirectory::CreateDirectory(m_sTempPath));
    }

    void TearDown() override
    {
        NSDirectory::DeleteDirectory(m_sDir);
    }

    // Returns the name the reader hands on, which is what GetBinFromStg returns.
    std::wstring ReadObject(const std::vector<BYTE>& arStream, const std::wstring& sName = L"")
    {
        const std::wstring sFile = m_sDir + FILE_SEPARATOR_STR + L"object.ppt";
        {
            POLE::Storage oStorage(sFile.c_str());
            if (!oStorage.open(true, true))
            {
                ADD_FAILURE() << "could not create the compound file";
                return L"";
            }

            POLE::Stream oStream(&oStorage, L"PowerPoint Document", true, (POLE::int64)arStream.size());
            oStream.write(const_cast<BYTE*>(arStream.data()), arStream.size());
            oStream.flush();
            oStorage.close();
        }

        POLE::Storage oStorage(sFile.c_str());
        if (!oStorage.open())
        {
            ADD_FAILURE() << "could not open the compound file";
            return L"";
        }

        POLE::Stream oStream(&oStorage, L"PowerPoint Document");
        if (oStream.size() != arStream.size())
        {
            ADD_FAILURE() << "the stream was not written in full";
            return L"";
        }

        SRecordHeader oHeader;
        if (!oHeader.ReadFromStream(&oStream))
        {
            ADD_FAILURE() << "could not read the record header";
            return L"";
        }

        CRecordExObjStg oRecord(sName, m_sTempPath);
        oRecord.ReadFromStream(oHeader, &oStream);
        return oRecord.m_sFileName;
    }

    std::vector<std::wstring> TempFiles()
    {
        return NSDirectory::GetFiles(m_sTempPath);
    }
};

// The reported overflow: with RecLen 0 the body length went through "0 - 4" and
// the read copied the rest of the stream into an empty buffer. The bytes after
// the record are what would have been written past it.
TEST_F(PptExObjStgTest, CompressedRecordWithNoBodyWritesNothing)
{
    std::vector<BYTE> arStream = MakeRecord(INSTANCE_COMPRESSED, 0, std::vector<BYTE>());
    arStream.insert(arStream.end(), 4096, 0x41);

    EXPECT_EQ(L"", ReadObject(arStream));
    EXPECT_TRUE(TempFiles().empty());
}

TEST_F(PptExObjStgTest, CompressedRecordWithOnlyTheSizeWritesNothing)
{
    std::vector<BYTE> arStream = MakeRecord(INSTANCE_COMPRESSED, MakeCompressedBody(1000, std::vector<BYTE>()));
    arStream.insert(arStream.end(), 4096, 0x41);

    EXPECT_EQ(L"", ReadObject(arStream));
    EXPECT_TRUE(TempFiles().empty());
}

// A few dozen stored bytes claiming 16 MiB: more than any deflate stream of that
// length can expand to, so the buffer is never allocated.
TEST_F(PptExObjStgTest, ClaimBeyondWhatTheDataCanHoldWritesNothing)
{
    const std::vector<BYTE> arCompressed = Compress(MakeObject(1000));
    ASSERT_LT(arCompressed.size(), 1000u);

    EXPECT_EQ(L"", ReadObject(MakeRecord(INSTANCE_COMPRESSED, MakeCompressedBody(16 * 1024 * 1024, arCompressed))));
    EXPECT_TRUE(TempFiles().empty());
}

// An object that really is 128 MiB once inflated, stored with enough noise in
// front that the inflation factor allows it, so only the size ceiling stops it.
TEST_F(PptExObjStgTest, ClaimAboveTheCeilingWritesNothing)
{
    std::vector<BYTE> arObject(128 * 1024 * 1024, 0);
    _UINT32 nSeed = 0x12345678;
    for (size_t i = 0; i < 256 * 1024; ++i)
    {
        nSeed = nSeed * 1103515245 + 12345;
        arObject[i] = (BYTE)(nSeed >> 16);
    }

    const std::vector<BYTE> arCompressed = Compress(arObject);
    ASSERT_GT((POLE::uint64)arCompressed.size() * 1032, (POLE::uint64)arObject.size() + 64);

    EXPECT_EQ(L"", ReadObject(MakeRecord(INSTANCE_COMPRESSED, MakeCompressedBody((_UINT32)arObject.size(), arCompressed))));
    EXPECT_TRUE(TempFiles().empty());
}

TEST_F(PptExObjStgTest, ObjectThatDoesNotInflateWritesNothing)
{
    const std::vector<BYTE> arGarbage(100, 0xAB);

    EXPECT_EQ(L"", ReadObject(MakeRecord(INSTANCE_COMPRESSED, MakeCompressedBody(1000, arGarbage))));
    EXPECT_TRUE(TempFiles().empty());
}

// vbaProject.bin is named by the caller, and a name that points at no file
// would still have made the converted document macro enabled.
TEST_F(PptExObjStgTest, RefusedNamedObjectIsNotCreated)
{
    std::vector<BYTE> arStream = MakeRecord(INSTANCE_COMPRESSED, 0, std::vector<BYTE>());
    arStream.insert(arStream.end(), 4096, 0x41);

    EXPECT_EQ(L"", ReadObject(arStream, L"vbaProject.bin"));
    EXPECT_TRUE(TempFiles().empty());
}

TEST_F(PptExObjStgTest, StoredObjectIsWritten)
{
    const std::vector<BYTE> arObject = MakeObject(5000);

    const std::wstring sFile = ReadObject(MakeRecord(INSTANCE_STORED, arObject));
    ASSERT_NE(L"", sFile);
    EXPECT_EQ(0u, sFile.find(m_sTempPath + FILE_SEPARATOR_STR));
    EXPECT_EQ(arObject, ReadAll(sFile));
    EXPECT_EQ(1u, TempFiles().size());
}

TEST_F(PptExObjStgTest, CompressedObjectIsWritten)
{
    const std::vector<BYTE> arObject = MakeObject(100000);

    const std::wstring sFile = ReadObject(MakeRecord(INSTANCE_COMPRESSED, MakeCompressedBody((_UINT32)arObject.size(), Compress(arObject))));
    ASSERT_NE(L"", sFile);
    EXPECT_EQ(arObject, ReadAll(sFile));
    EXPECT_EQ(1u, TempFiles().size());
}

// The file holds what the inflate produced, not the size the record claimed.
TEST_F(PptExObjStgTest, CompressedObjectKeepsItsInflatedSize)
{
    const std::vector<BYTE> arObject = MakeObject(100000);

    const std::wstring sFile = ReadObject(MakeRecord(INSTANCE_COMPRESSED, MakeCompressedBody((_UINT32)arObject.size() + 1000, Compress(arObject))));
    ASSERT_NE(L"", sFile);
    EXPECT_EQ(arObject, ReadAll(sFile));
}

TEST_F(PptExObjStgTest, NamedObjectKeepsOnlyTheLastPartOfItsName)
{
    const std::vector<BYTE> arObject = MakeObject(5000);

    const std::wstring sFile = ReadObject(MakeRecord(INSTANCE_STORED, arObject), L"../outside/vbaProject.bin");
    EXPECT_EQ(m_sTempPath + FILE_SEPARATOR_STR + L"vbaProject.bin", sFile);
    EXPECT_EQ(arObject, ReadAll(sFile));
    EXPECT_FALSE(NSDirectory::Exists(m_sDir + FILE_SEPARATOR_STR + L"outside"));
}
