/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */
#include "ExObjListContainer.h"

#include <vector>

using namespace PPT;

namespace
{
    // An embedded object is a single object, never a stream of the document, so a
    // claimed size far above the stored size means the file is lying. The ceiling
    // has to be a multiple of the stored size rather than something tight: a
    // deflate stream pays a match code, not the bytes it covers, and a match is up
    // to 258 bytes long, so a hand made stream can turn one stored byte into about
    // a thousand output bytes, 1032 at most. No producer of real files goes past
    // that, which is what makes the factor a useful line to draw. It is not
    // compressBound, which bounds the other direction - what compress() may grow
    // to - and a real object that happens to compress well would be refused if it
    // were used here.
    const POLE::uint64 MAX_INFLATION_FACTOR = 1032;

    // Ceiling for the object, whatever the header claims.
    const POLE::uint64 MAX_EX_OBJ_STG_SIZE = 512 * 1024 * 1024;

    // Both callers pass a constant name today. It is still cut to its last part,
    // so that a name ever taken from the file cannot carry a path: whatever
    // precedes the last separator is dropped, with either separator, and a name
    // that is left empty or is a relative hop gets no name at all.
    std::wstring GetFilePart(const std::wstring& strName)
    {
        std::wstring::size_type nPos = strName.find_last_of(L"/\\");
        std::wstring sResult = (std::wstring::npos == nPos) ? strName : strName.substr(nPos + 1);

        if (sResult.empty() || L"." == sResult || L".." == sResult)
            return std::wstring();

        return sResult;
    }
}

void CRecordExObjListContainer::ReadFromStream(SRecordHeader &oHeader, POLE::Stream *pStream)
{
    CRecordsContainer::ReadFromStream(oHeader, pStream);
}

CRecordExObjStg::CRecordExObjStg(const std::wstring& name, const std::wstring& tempPath)
{
    const std::wstring sName = GetFilePart(name);

    if (sName.empty())
    {
        m_sFileName = NSFile::CFileBinary::CreateTempFileWithUniqueName(tempPath, L"bin");
    }
    else
    {
        m_sFileName = tempPath + FILE_SEPARATOR_STR + sName;
    }
}

CRecordExObjStg::~CRecordExObjStg()
{
}

void CRecordExObjStg::ReadFromStream(SRecordHeader& oHeader, POLE::Stream* pStream)
{
    m_oHeader = oHeader;

    // Everything below is sized from the header and from the object itself, both
    // of which an attacker writes. Sizes are therefore checked against the bytes
    // that are actually left in the stream before they are allocated or read, and
    // nothing is written to disk unless the stored data was read and inflated
    // successfully. A bad header is not refused here: the header reader has
    // already cut RecLen down to what is left in the stream, and real files reach
    // this point with that flag set.
    //
    // m_sFileName is what the caller hands on, and an object it names is taken to
    // exist (vbaProject.bin turns the document macro enabled). So it names the
    // file only once the object is in it, and every refusal leaves it empty.
    const std::wstring sFileName = m_sFileName;
    m_sFileName.clear();

    if (NULL == pStream)
        return;

    const bool bCompressed = (0x01 == m_oHeader.RecInstance);

    const POLE::uint64 nAvailable = pStream->size() - pStream->tell();
    if ((POLE::uint64)m_oHeader.RecLen > nAvailable)
        return;

    // dwUncompressedSize is the first four bytes of the record when it is there,
    // so a record that holds nothing but those bytes has no data at all.
    if (bCompressed && m_oHeader.RecLen < 4)
        return;

    ULONG compressedSize = m_oHeader.RecLen;
    ULONG decompressedSize = m_oHeader.RecLen;

    if (bCompressed)
    {
        const _UINT32 nClaimedSize = StreamUtils::ReadDWORD(pStream);
        compressedSize -= 4;

        const POLE::uint64 nNeeded = (POLE::uint64)nClaimedSize + 64;
        if (nNeeded > MAX_EX_OBJ_STG_SIZE)
            return;

        if (nNeeded > (POLE::uint64)compressedSize * MAX_INFLATION_FACTOR)
            return;

        decompressedSize = (ULONG)nNeeded;
    }

    if (0 == compressedSize)
        return;

    std::vector<BYTE> arData(compressedSize);
    if (pStream->read(arData.data(), compressedSize) != (POLE::uint64)compressedSize)
        return;

    if (bCompressed)
    {
        std::vector<BYTE> arUncompressed(decompressedSize, 0);
        ULONG nUncompressedSize = decompressedSize;

        if (!NSZip::Decompress(arData.data(), compressedSize, arUncompressed.data(), nUncompressedSize))
            return;

        // uncompress() reports the size it produced, which is the only size worth
        // keeping: the claimed size above it was never filled.
        arUncompressed.resize(nUncompressedSize);
        arData.swap(arUncompressed);
    }

    if (arData.empty())
        return;

    NSFile::CFileBinary file;
    if (file.CreateFileW(sFileName))
    {
        file.WriteFile(arData.data(), (ULONG)arData.size());
        file.CloseFile();
        m_sFileName = sFileName;
    }
}
