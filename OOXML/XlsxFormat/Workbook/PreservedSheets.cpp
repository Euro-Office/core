/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include "PreservedSheets.h"

#include "Workbook.h"

#include "../../Common/SimpleTypes_Shared.h"
#include "../../Common/SimpleTypes_Spreadsheet.h"

#include "../../DocxFormat/ContentTypes.h"
#include "../../DocxFormat/File.h"
#include "../../DocxFormat/FileType.h"
#include "../../DocxFormat/Rels.h"

#include "../../../DesktopEditor/common/Directory.h"
#include "../../../DesktopEditor/common/File.h"
#include "../../../DesktopEditor/xml/include/xmlutils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <deque>
#include <map>
#include <set>

namespace OOX
{
namespace Spreadsheet
{
namespace
{
	const wchar_t* c_sRelsContentType = L"application/vnd.openxmlformats-package.relationships+xml";

	std::wstring ToLower(std::wstring s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return (wchar_t)std::towlower(c); });
		return s;
	}
	std::wstring NormalizeSlashes(std::wstring s)
	{
		std::replace(s.begin(), s.end(), L'\\', L'/');
		return s;
	}
	std::wstring DirName(const std::wstring& sPath)
	{
		size_t nPos = sPath.rfind(L'/');
		return std::wstring::npos == nPos ? L"" : sPath.substr(0, nPos);
	}
	std::wstring FileName(const std::wstring& sPath)
	{
		size_t nPos = sPath.rfind(L'/');
		return std::wstring::npos == nPos ? sPath : sPath.substr(nPos + 1);
	}
	std::vector<std::wstring> SplitPath(const std::wstring& sPath)
	{
		std::vector<std::wstring> arParts;
		size_t nStart = 0;
		while (nStart <= sPath.size())
		{
			size_t nEnd = sPath.find(L'/', nStart);
			if (std::wstring::npos == nEnd)
				nEnd = sPath.size();
			std::wstring sPart = sPath.substr(nStart, nEnd - nStart);
			if (sPart == L"..")
			{
				if (!arParts.empty())
					arParts.pop_back();
			}
			else if (!sPart.empty() && sPart != L".")
				arParts.push_back(sPart);
			nStart = nEnd + 1;
		}
		return arParts;
	}
	std::wstring JoinParts(const std::vector<std::wstring>& arParts, size_t nFrom = 0)
	{
		std::wstring sResult;
		for (size_t i = nFrom; i < arParts.size(); ++i)
		{
			if (!sResult.empty())
				sResult += L'/';
			sResult += arParts[i];
		}
		return sResult;
	}
	// Resolves a relationship target against the directory of its source part.
	std::wstring ResolvePart(const std::wstring& sSourceDir, const std::wstring& sTarget)
	{
		std::wstring sTargetNorm = NormalizeSlashes(sTarget);
		if (!sTargetNorm.empty() && sTargetNorm[0] == L'/')
			return JoinParts(SplitPath(sTargetNorm));
		return JoinParts(SplitPath(sSourceDir + L"/" + sTargetNorm));
	}
	std::wstring RelativePart(const std::wstring& sFromDir, const std::wstring& sToPart)
	{
		std::vector<std::wstring> arFrom = SplitPath(sFromDir);
		std::vector<std::wstring> arTo = SplitPath(sToPart);
		size_t nCommon = 0;
		while (nCommon < arFrom.size() && nCommon + 1 < arTo.size() && arFrom[nCommon] == arTo[nCommon])
			++nCommon;
		std::wstring sResult;
		for (size_t i = nCommon; i < arFrom.size(); ++i)
			sResult += L"../";
		return sResult + JoinParts(arTo, nCommon);
	}
	std::wstring RelsPartName(const std::wstring& sPart)
	{
		std::wstring sDir = DirName(sPart);
		return (sDir.empty() ? L"" : sDir + L"/") + L"_rels/" + FileName(sPart) + L".rels";
	}
	// "xl/drawings/_rels/drawing1.xml.rels" -> "xl/drawings/drawing1.xml"
	std::wstring RelsOwnerPart(const std::wstring& sRelsPart)
	{
		std::wstring sRelsDir = DirName(sRelsPart);
		std::wstring sName = FileName(sRelsPart);
		if (FileName(sRelsDir) != L"_rels" || sName.size() <= 5)
			return L"";
		std::wstring sDir = DirName(sRelsDir);
		std::wstring sOwner = sName.substr(0, sName.size() - 5);
		return sDir.empty() ? sOwner : sDir + L"/" + sOwner;
	}

	bool ReadPackageFile(const std::wstring& sPackageDir, const std::wstring& sPart, std::string& sData)
	{
		std::wstring sPath = sPackageDir + FILE_SEPARATOR_STR + sPart;
		if (!NSFile::CFileBinary::Exists(sPath))
			return false;
		BYTE* pData = NULL;
		DWORD nSize = 0;
		if (!NSFile::CFileBinary::ReadAllBytes(sPath, &pData, nSize))
			return false;
		sData.assign((const char*)pData, nSize);
		RELEASEARRAYOBJECTS(pData);
		return true;
	}

	// ----- minimal markup scanner (the parts are UTF-8; all markup is ASCII) -----

	struct CTag
	{
		size_t nStart = std::string::npos; // position of '<'
		size_t nEnd = std::string::npos;   // position after '>'
		std::string sName;                 // local name, without namespace prefix
		std::string sPrefix;               // "x:" or empty
		bool bSelfClosing = false;
	};
	bool NextStartTag(const std::string& sXml, size_t nFrom, CTag& oTag)
	{
		size_t nPos = nFrom;
		while (true)
		{
			nPos = sXml.find('<', nPos);
			if (std::string::npos == nPos || nPos + 1 >= sXml.size())
				return false;
			char c = sXml[nPos + 1];
			if (c == '/' || c == '?' || c == '!')
			{
				++nPos;
				continue;
			}
			size_t nNameEnd = nPos + 1;
			while (nNameEnd < sXml.size() && !isspace((unsigned char)sXml[nNameEnd]) && sXml[nNameEnd] != '>' && sXml[nNameEnd] != '/')
				++nNameEnd;
			std::string sQName = sXml.substr(nPos + 1, nNameEnd - nPos - 1);
			char cQuote = 0;
			size_t nEnd = nNameEnd;
			for (; nEnd < sXml.size(); ++nEnd)
			{
				char ch = sXml[nEnd];
				if (cQuote)
				{
					if (ch == cQuote)
						cQuote = 0;
				}
				else if (ch == '"' || ch == '\'')
					cQuote = ch;
				else if (ch == '>')
					break;
			}
			if (nEnd >= sXml.size())
				return false;
			size_t nColon = sQName.find(':');
			oTag.nStart = nPos;
			oTag.nEnd = nEnd + 1;
			oTag.sPrefix = std::string::npos == nColon ? "" : sQName.substr(0, nColon + 1);
			oTag.sName = std::string::npos == nColon ? sQName : sQName.substr(nColon + 1);
			oTag.bSelfClosing = sXml[nEnd - 1] == '/';
			return true;
		}
	}
	// Finds attribute sName in a start tag; returns the value range [nValueStart, nValueEnd).
	bool FindAttr(const std::string& sTag, const std::string& sName, size_t& nAttrStart, size_t& nValueStart, size_t& nValueEnd)
	{
		size_t nPos = 0;
		while (true)
		{
			nPos = sTag.find(sName, nPos);
			if (std::string::npos == nPos)
				return false;
			size_t nAfter = nPos + sName.size();
			bool bBoundary = nPos > 0 && isspace((unsigned char)sTag[nPos - 1]);
			size_t nEq = nAfter;
			while (nEq < sTag.size() && isspace((unsigned char)sTag[nEq]))
				++nEq;
			if (bBoundary && nEq < sTag.size() && sTag[nEq] == '=')
			{
				size_t nQuote = nEq + 1;
				while (nQuote < sTag.size() && isspace((unsigned char)sTag[nQuote]))
					++nQuote;
				if (nQuote < sTag.size() && (sTag[nQuote] == '"' || sTag[nQuote] == '\''))
				{
					size_t nClose = sTag.find(sTag[nQuote], nQuote + 1);
					if (std::string::npos == nClose)
						return false;
					nAttrStart = nPos - 1; // include the preceding whitespace
					nValueStart = nQuote + 1;
					nValueEnd = nClose;
					return true;
				}
			}
			nPos = nAfter;
		}
	}
	bool GetAttr(const std::string& sTag, const std::string& sName, std::string& sValue)
	{
		size_t nAttrStart, nValueStart, nValueEnd;
		if (!FindAttr(sTag, sName, nAttrStart, nValueStart, nValueEnd))
			return false;
		sValue = sTag.substr(nValueStart, nValueEnd - nValueStart);
		return true;
	}
	void RemoveAttr(std::string& sTag, const std::string& sName)
	{
		size_t nAttrStart, nValueStart, nValueEnd;
		if (FindAttr(sTag, sName, nAttrStart, nValueStart, nValueEnd))
			sTag.erase(nAttrStart, nValueEnd + 1 - nAttrStart);
	}
	void SetAttrValue(std::string& sTag, const std::string& sName, const std::string& sValue)
	{
		size_t nAttrStart, nValueStart, nValueEnd;
		if (FindAttr(sTag, sName, nAttrStart, nValueStart, nValueEnd))
			sTag.replace(nValueStart, nValueEnd - nValueStart, sValue);
	}
	std::string DecodeXml(const std::string& s)
	{
		std::string sResult;
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] == '&')
			{
				static const char* arEntities[][2] = {{"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}};
				bool bFound = false;
				for (auto& entity : arEntities)
				{
					if (0 == s.compare(i, strlen(entity[0]), entity[0]))
					{
						sResult += entity[1];
						i += strlen(entity[0]) - 1;
						bFound = true;
						break;
					}
				}
				if (bFound)
					continue;
			}
			sResult += s[i];
		}
		return sResult;
	}
	std::string EncodeXml(const std::string& s)
	{
		std::string sResult;
		for (char c : s)
		{
			switch (c)
			{
			case '&': sResult += "&amp;"; break;
			case '<': sResult += "&lt;"; break;
			case '>': sResult += "&gt;"; break;
			case '"': sResult += "&quot;"; break;
			default: sResult += c;
			}
		}
		return sResult;
	}
	std::wstring Utf8ToW(const std::string& s)
	{
		return NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)s.c_str(), (LONG)s.size());
	}
	std::string WToUtf8(const std::wstring& s)
	{
		return NSFile::CUtf8Converter::GetUtf8StringFromUnicode2(s.c_str(), (LONG)s.size());
	}

	struct CRelationship
	{
		std::string sTarget; // decoded
		bool bExternal = false;
	};
	std::vector<CRelationship> ParseRelationships(const std::string& sXml)
	{
		std::vector<CRelationship> arRels;
		CTag oTag;
		size_t nPos = 0;
		while (NextStartTag(sXml, nPos, oTag))
		{
			nPos = oTag.nEnd;
			if (oTag.sName != "Relationship")
				continue;
			std::string sTag = sXml.substr(oTag.nStart, oTag.nEnd - oTag.nStart);
			CRelationship oRel;
			std::string sMode;
			if (!GetAttr(sTag, "Target", oRel.sTarget))
				continue;
			oRel.sTarget = DecodeXml(oRel.sTarget);
			oRel.bExternal = GetAttr(sTag, "TargetMode", sMode) && sMode == "External";
			arRels.push_back(oRel);
		}
		return arRels;
	}

	class CContentTypeMap
	{
	public:
		explicit CContentTypeMap(const std::wstring& sPackageDir)
		{
			std::string sXml;
			if (!ReadPackageFile(sPackageDir, L"[Content_Types].xml", sXml))
				return;
			CTag oTag;
			size_t nPos = 0;
			while (NextStartTag(sXml, nPos, oTag))
			{
				nPos = oTag.nEnd;
				std::string sTag = sXml.substr(oTag.nStart, oTag.nEnd - oTag.nStart);
				std::string sKey, sType;
				if (!GetAttr(sTag, "ContentType", sType))
					continue;
				if (oTag.sName == "Default" && GetAttr(sTag, "Extension", sKey))
					m_mapDefaults[ToLower(Utf8ToW(DecodeXml(sKey)))] = Utf8ToW(DecodeXml(sType));
				else if (oTag.sName == "Override" && GetAttr(sTag, "PartName", sKey))
					m_mapOverrides[ToLower(ResolvePart(L"", Utf8ToW(DecodeXml(sKey))))] = Utf8ToW(DecodeXml(sType));
			}
		}
		std::wstring Get(const std::wstring& sPart) const
		{
			auto itOverride = m_mapOverrides.find(ToLower(sPart));
			if (itOverride != m_mapOverrides.end())
				return itOverride->second;
			std::wstring sName = FileName(sPart);
			size_t nDot = sName.rfind(L'.');
			if (std::wstring::npos != nDot)
			{
				auto itDefault = m_mapDefaults.find(ToLower(sName.substr(nDot + 1)));
				if (itDefault != m_mapDefaults.end())
					return itDefault->second;
			}
			return L"application/octet-stream";
		}
	private:
		std::map<std::wstring, std::wstring> m_mapDefaults;
		std::map<std::wstring, std::wstring> m_mapOverrides;
	};

	// Copies the sheet part and everything reachable through its internal relationships.
	bool CollectParts(const std::wstring& sPackageDir, const std::wstring& sRootPart, const CContentTypeMap& oContentTypes, std::vector<CPreservedPart>& arParts)
	{
		std::set<std::wstring> setSeen;
		std::deque<std::wstring> arQueue(1, sRootPart);
		while (!arQueue.empty())
		{
			std::wstring sPart = arQueue.front();
			arQueue.pop_front();
			if (!setSeen.insert(ToLower(sPart)).second)
				continue;

			CPreservedPart oPart;
			if (!ReadPackageFile(sPackageDir, sPart, oPart.sData))
			{
				if (sPart == sRootPart)
					return false;
				continue;
			}
			oPart.sName = sPart;
			oPart.sContentType = oContentTypes.Get(sPart);
			arParts.push_back(oPart);

			CPreservedPart oRels;
			oRels.sName = RelsPartName(sPart);
			if (ReadPackageFile(sPackageDir, oRels.sName, oRels.sData))
			{
				for (const CRelationship& oRel : ParseRelationships(oRels.sData))
				{
					if (!oRel.bExternal)
						arQueue.push_back(ResolvePart(DirName(sPart), Utf8ToW(oRel.sTarget)));
				}
				arParts.push_back(oRels);
			}
		}
		return true;
	}

	std::vector<std::string> ReadSharedStrings(const std::wstring& sPackageDir, const std::wstring& sPart)
	{
		std::vector<std::string> arItems;
		std::string sXml;
		if (sPart.empty() || !ReadPackageFile(sPackageDir, sPart, sXml))
			return arItems;
		CTag oTag;
		size_t nPos = 0;
		while (NextStartTag(sXml, nPos, oTag))
		{
			nPos = oTag.nEnd;
			if (oTag.sName != "si")
				continue;
			if (oTag.bSelfClosing)
			{
				arItems.push_back("");
				continue;
			}
			std::string sClose = "</" + oTag.sPrefix + "si>";
			size_t nClose = sXml.find(sClose, oTag.nEnd);
			if (std::string::npos == nClose)
				break;
			arItems.push_back(sXml.substr(oTag.nEnd, nClose - oTag.nEnd));
			nPos = nClose + sClose.size();
		}
		return arItems;
	}

	// Makes the sheet part independent of workbook-level tables the editor rewrites on
	// save: shared-string cells become inline strings, style indices are dropped (they
	// would point at different or missing formats), and the tab is no longer selected
	// (Excel would otherwise open with two sheets grouped).
	void DetachSheetXml(std::string& sXml, const std::vector<std::string>& arSharedStrings)
	{
		std::string sResult;
		sResult.reserve(sXml.size());
		size_t nCopied = 0;
		CTag oTag;
		size_t nPos = 0;
		while (NextStartTag(sXml, nPos, oTag))
		{
			nPos = oTag.nEnd;
			if (oTag.sName != "c" && oTag.sName != "row" && oTag.sName != "col" && oTag.sName != "sheetView")
				continue;

			std::string sTag = sXml.substr(oTag.nStart, oTag.nEnd - oTag.nStart);
			size_t nReplaceEnd = oTag.nEnd;
			if (oTag.sName == "row")
			{
				RemoveAttr(sTag, "s");
				RemoveAttr(sTag, "customFormat");
			}
			else if (oTag.sName == "col")
				RemoveAttr(sTag, "style");
			else if (oTag.sName == "sheetView")
				RemoveAttr(sTag, "tabSelected");
			else
			{
				RemoveAttr(sTag, "s");
				std::string sType;
				if (!oTag.bSelfClosing && GetAttr(sTag, "t", sType) && sType == "s")
				{
					std::string sClose = "</" + oTag.sPrefix + "c>";
					size_t nClose = sXml.find(sClose, oTag.nEnd);
					std::string sOpenV = "<" + oTag.sPrefix + "v>", sCloseV = "</" + oTag.sPrefix + "v>";
					size_t nV = sXml.find(sOpenV, oTag.nEnd);
					size_t nVEnd = std::string::npos == nV ? std::string::npos : sXml.find(sCloseV, nV);
					if (std::string::npos != nClose && std::string::npos != nVEnd && nVEnd < nClose)
					{
						std::string sIndex = sXml.substr(nV + sOpenV.size(), nVEnd - nV - sOpenV.size());
						size_t nIndex = (size_t)atol(sIndex.c_str());
						if (!sIndex.empty() && nIndex < arSharedStrings.size())
						{
							SetAttrValue(sTag, "t", "inlineStr");
							sTag += sXml.substr(oTag.nEnd, nV - oTag.nEnd);
							sTag += "<" + oTag.sPrefix + "is>" + arSharedStrings[nIndex] + "</" + oTag.sPrefix + "is>";
							sTag += sXml.substr(nVEnd + sCloseV.size(), nClose - nVEnd - sCloseV.size());
							nReplaceEnd = nClose;
							nPos = nClose;
						}
					}
				}
			}
			sResult.append(sXml, nCopied, oTag.nStart - nCopied);
			sResult += sTag;
			nCopied = nReplaceEnd;
		}
		sResult.append(sXml, nCopied, std::string::npos);
		sXml.swap(sResult);
	}

	// Whether a defined name's formula refers to the given sheet ("Name!..." or "'Na''me'!...").
	bool RefersToSheet(const std::wstring& sRef, const std::wstring& sSheetName)
	{
		if (sSheetName.empty())
			return false;
		std::wstring sRefLower = ToLower(sRef);
		std::wstring sNameLower = ToLower(sSheetName);
		std::wstring sQuoted = L"'";
		for (wchar_t c : sNameLower)
		{
			sQuoted += c;
			if (c == L'\'')
				sQuoted += c;
		}
		sQuoted += L"'!";
		if (std::wstring::npos != sRefLower.find(sQuoted))
			return true;
		std::wstring sPlain = sNameLower + L"!";
		size_t nPos = 0;
		while (std::wstring::npos != (nPos = sRefLower.find(sPlain, nPos)))
		{
			wchar_t cPrev = nPos > 0 ? sRefLower[nPos - 1] : L' ';
			if (!std::iswalnum(cPrev) && cPrev != L'_' && cPrev != L'.' && cPrev != L'\'')
				return true;
			nPos += sPlain.size();
		}
		return false;
	}

	// Drops a prefix added by an earlier save ("eoMacro2_drawing1.xml" -> "drawing1.xml"),
	// so names do not grow with every round trip.
	std::wstring StripPreservedPrefix(const std::wstring& sFileName)
	{
		const std::wstring sPrefix = L"eoMacro";
		if (0 != sFileName.compare(0, sPrefix.size(), sPrefix))
			return sFileName;
		size_t nPos = sPrefix.size();
		while (nPos < sFileName.size() && std::iswdigit(sFileName[nPos]))
			++nPos;
		if (nPos == sPrefix.size() || nPos >= sFileName.size() || sFileName[nPos] != L'_')
			return sFileName;
		return sFileName.substr(nPos + 1);
	}

	// A sheet part written back exactly as read; related parts get prefixed names so
	// they cannot collide with parts the regular writer creates.
	class CPreservedSheetFile : public OOX::File
	{
	public:
		CPreservedSheetFile(OOX::Document* pMain, const CPreservedSheet& oSheet, int nNumber)
			: OOX::File(pMain), m_oSheet(oSheet), m_nNumber(nNumber)
		{
			bool bIntl = std::wstring::npos != m_oSheet.sRelType.find(L"xlIntlMacrosheet");
			m_sFileName = (bIntl ? L"intlsheet" : L"sheet") + std::to_wstring(nNumber) + L".xml";
		}
		virtual void read(const CPath&) {}
		virtual void write(const CPath& oPath, const CPath& oDirectory, CContentTypes& oContent) const
		{
			if (m_oSheet.arParts.empty())
				return;

			std::wstring sDirectory = JoinParts(SplitPath(NormalizeSlashes(oDirectory.GetPath())));
			std::wstring sRootNew = sDirectory + L"/" + m_sFileName;
			std::wstring sOutPath = NormalizeSlashes(oPath.GetPath());
			std::wstring sOutRoot = sOutPath.size() > sRootNew.size() + 1 ? sOutPath.substr(0, sOutPath.size() - sRootNew.size() - 1) : L"";
			if (sOutRoot.empty() || 0 != sOutPath.compare(sOutRoot.size() + 1, std::wstring::npos, sRootNew))
			{
				// Unexpected layout: still write the sheet part so the workbook relationship stays valid.
				const CPreservedPart& oRoot = m_oSheet.arParts[0];
				NSFile::CFileBinary oFile;
				if (oFile.CreateFileW(oPath.GetPath()))
				{
					oFile.WriteFile((const BYTE*)oRoot.sData.c_str(), (DWORD)oRoot.sData.size());
					oFile.CloseFile();
					oContent.Registration(oRoot.sContentType, oDirectory, oPath.GetFilename());
				}
				return;
			}

			// original part name -> written part name
			std::map<std::wstring, std::wstring> mapNames;
			std::wstring sPrefix = L"eoMacro" + std::to_wstring(m_nNumber) + L"_";
			mapNames[m_oSheet.arParts[0].sName] = sRootNew;
			for (size_t i = 1; i < m_oSheet.arParts.size(); ++i)
			{
				const std::wstring& sName = m_oSheet.arParts[i].sName;
				if (RelsOwnerPart(sName).empty())
					mapNames[sName] = (DirName(sName).empty() ? L"" : DirName(sName) + L"/") + sPrefix + StripPreservedPrefix(FileName(sName));
			}

			for (const CPreservedPart& oPart : m_oSheet.arParts)
			{
				std::wstring sOwner = RelsOwnerPart(oPart.sName);
				bool bRels = !sOwner.empty() && mapNames.end() != mapNames.find(sOwner);
				std::wstring sNewName = bRels ? RelsPartName(mapNames[sOwner]) : mapNames[oPart.sName];
				if (sNewName.empty())
					continue;

				std::string sData = bRels ? RewriteTargets(oPart.sData, sOwner, mapNames[sOwner], mapNames) : oPart.sData;
				std::wstring sFilePath = sOutRoot + L"/" + sNewName;
				NSDirectory::CreateDirectories(DirName(sFilePath));
				NSFile::CFileBinary oFile;
				if (!oFile.CreateFileW(sFilePath))
					continue;
				oFile.WriteFile((const BYTE*)sData.c_str(), (DWORD)sData.size());
				oFile.CloseFile();

				if (!bRels)
					oContent.Registration(oPart.sContentType, OOX::CPath(DirName(sNewName)), OOX::CPath(FileName(sNewName)));
			}
		}
		virtual const OOX::FileType type() const
		{
			return OOX::FileType(L"macrosheets", m_sFileName, m_oSheet.arParts.empty() ? L"" : m_oSheet.arParts[0].sContentType, m_oSheet.sRelType);
		}
		virtual const CPath DefaultDirectory() const
		{
			return CPath(std::wstring(L"macrosheets"));
		}
		virtual const CPath DefaultFileName() const
		{
			return m_sFileName;
		}
	private:
		static std::string RewriteTargets(const std::string& sXml, const std::wstring& sOwnerOld, const std::wstring& sOwnerNew,
										  const std::map<std::wstring, std::wstring>& mapNames)
		{
			std::string sResult;
			size_t nCopied = 0;
			CTag oTag;
			size_t nPos = 0;
			while (NextStartTag(sXml, nPos, oTag))
			{
				nPos = oTag.nEnd;
				if (oTag.sName != "Relationship")
					continue;
				std::string sTag = sXml.substr(oTag.nStart, oTag.nEnd - oTag.nStart);
				std::string sTarget, sMode;
				if (!GetAttr(sTag, "Target", sTarget) || (GetAttr(sTag, "TargetMode", sMode) && sMode == "External"))
					continue;
				auto itNew = mapNames.find(ResolvePart(DirName(sOwnerOld), Utf8ToW(DecodeXml(sTarget))));
				if (itNew == mapNames.end())
					continue;
				SetAttrValue(sTag, "Target", EncodeXml(WToUtf8(RelativePart(DirName(sOwnerNew), itNew->second))));
				sResult.append(sXml, nCopied, oTag.nStart - nCopied);
				sResult += sTag;
				nCopied = oTag.nEnd;
			}
			sResult.append(sXml, nCopied, std::string::npos);
			return sResult;
		}

		CPreservedSheet m_oSheet;
		std::wstring m_sFileName;
		int m_nNumber;
	};

	// Index in the editor's sheet list for an index in the original list.
	unsigned int EditorIndex(unsigned int nOriginal, const std::vector<CPreservedSheet>& arSheets, size_t nEditorCount)
	{
		unsigned int nBefore = 0;
		for (const CPreservedSheet& oSheet : arSheets)
		{
			if ((unsigned int)oSheet.nIndex < nOriginal)
				++nBefore;
		}
		unsigned int nIndex = nOriginal - nBefore;
		if (nEditorCount > 0 && nIndex >= nEditorCount)
			nIndex = (unsigned int)nEditorCount - 1;
		return nIndex;
	}
}

bool IsPreservedSheetRelType(const std::wstring& sRelType)
{
	return sRelType == L"http://schemas.microsoft.com/office/2006/relationships/xlMacrosheet" ||
		   sRelType == L"http://schemas.microsoft.com/office/2006/relationships/xlIntlMacrosheet";
}

void CollectPreservedSheets(CWorkbook& workbook, const std::wstring& sPackageDir)
{
	workbook.m_arPreservedSheets.clear();
	if (!workbook.m_oSheets.IsInit() || !workbook.m_pCurRels.IsInit())
		return;

	std::wstring sPackageRoot = JoinParts(SplitPath(NormalizeSlashes(sPackageDir)));
	std::wstring sReadPath = JoinParts(SplitPath(NormalizeSlashes(workbook.m_oReadPath.GetPath())));
	if (sReadPath.size() <= sPackageRoot.size() || 0 != sReadPath.compare(0, sPackageRoot.size(), sPackageRoot))
		return;
	std::wstring sWorkbookPart = sReadPath.substr(sPackageRoot.size() + 1);
	if (ToLower(sWorkbookPart.substr(sWorkbookPart.size() > 4 ? sWorkbookPart.size() - 4 : 0)) != L".xml")
		return; // xlsb: parts are binary records, nothing to preserve here
	std::wstring sWorkbookDir = DirName(sWorkbookPart);
	if (!sPackageDir.empty() && (sPackageDir[0] == L'/' || sPackageDir[0] == L'\\'))
		sPackageRoot = L"/" + sPackageRoot;

	std::map<std::wstring, OOX::Rels::CRelationShip*> mapRels;
	std::wstring sSharedStringsPart;
	for (OOX::Rels::CRelationShip* pRel : workbook.m_pCurRels->m_arRelations)
	{
		if (!pRel)
			continue;
		mapRels[pRel->rId().ToString()] = pRel;
		if (!pRel->IsExternal() && std::wstring::npos != pRel->Type().find(L"/sharedStrings"))
			sSharedStringsPart = ResolvePart(sWorkbookDir, pRel->Target().GetPath());
	}

	CContentTypeMap oContentTypes(sPackageRoot);
	std::vector<std::string> arSharedStrings;
	bool bSharedStringsRead = false;

	std::vector<CSheet*>& arItems = workbook.m_oSheets->m_arrItems;
	for (size_t i = 0; i < arItems.size(); ++i)
	{
		CSheet* pSheet = arItems[i];
		if (!pSheet || !pSheet->m_oRid.IsInit())
			continue;
		auto itRel = mapRels.find(pSheet->m_oRid->GetValue());
		if (itRel == mapRels.end() || itRel->second->IsExternal() || !IsPreservedSheetRelType(itRel->second->Type()))
			continue;

		CPreservedSheet oSheet;
		oSheet.sName = pSheet->m_oName.get_value_or(L"");
		oSheet.sState = pSheet->m_oState.IsInit() ? pSheet->m_oState->ToString() : L"";
		oSheet.sRelType = itRel->second->Type();
		oSheet.nSheetId = pSheet->m_oSheetId.IsInit() ? pSheet->m_oSheetId->GetValue() : 0;
		oSheet.nIndex = (int)i;
		if (!CollectParts(sPackageRoot, ResolvePart(sWorkbookDir, itRel->second->Target().GetPath()), oContentTypes, oSheet.arParts))
			continue;

		if (!bSharedStringsRead)
		{
			arSharedStrings = ReadSharedStrings(sPackageRoot, sSharedStringsPart);
			bSharedStringsRead = true;
		}
		DetachSheetXml(oSheet.arParts[0].sData, arSharedStrings);
		workbook.m_arPreservedSheets.push_back(oSheet);
	}

	std::vector<CPreservedSheet>& arSheets = workbook.m_arPreservedSheets;
	if (arSheets.empty())
		return;
	size_t nEditorCount = arItems.size() - arSheets.size();

	// Names belonging to a preserved sheet travel with it; the editor would turn them into #REF!.
	if (workbook.m_oDefinedNames.IsInit())
	{
		std::vector<CDefinedName*> arKeep;
		for (CDefinedName* pName : workbook.m_oDefinedNames->m_arrItems)
		{
			if (!pName)
				continue;
			int nOwner = -1;
			bool bLocal = false;
			for (size_t k = 0; k < arSheets.size() && nOwner < 0; ++k)
			{
				if (pName->m_oLocalSheetId.IsInit() && pName->m_oLocalSheetId->GetValue() == (unsigned int)arSheets[k].nIndex)
				{
					nOwner = (int)k;
					bLocal = true;
				}
				else if (pName->m_oRef.IsInit() && RefersToSheet(*pName->m_oRef, arSheets[k].sName))
					nOwner = (int)k;
			}
			if (nOwner >= 0)
			{
				NSStringUtils::CStringBuilder oWriter;
				pName->toXML(oWriter); // the no-argument toXML() of CDefinedName returns an empty string
				CPreservedDefinedName oName;
				oName.sXml = oWriter.GetData();
				oName.bLocal = bLocal;
				arSheets[nOwner].arDefinedNames.push_back(oName);
				delete pName;
				continue;
			}
			if (pName->m_oLocalSheetId.IsInit())
				pName->m_oLocalSheetId->SetValue(EditorIndex(pName->m_oLocalSheetId->GetValue(), arSheets, nEditorCount));
			arKeep.push_back(pName);
		}
		workbook.m_oDefinedNames->m_arrItems = arKeep;
	}
	if (workbook.m_oBookViews.IsInit())
	{
		for (CWorkbookView* pView : workbook.m_oBookViews->m_arrItems)
		{
			if (pView && pView->m_oActiveTab.IsInit())
				pView->m_oActiveTab->SetValue(EditorIndex(pView->m_oActiveTab->GetValue(), arSheets, nEditorCount));
			if (pView && pView->m_oFirstSheet.IsInit())
				pView->m_oFirstSheet->SetValue(EditorIndex(pView->m_oFirstSheet->GetValue(), arSheets, nEditorCount));
		}
	}
	for (size_t k = arSheets.size(); k-- > 0;)
	{
		delete arItems[arSheets[k].nIndex];
		arItems.erase(arItems.begin() + arSheets[k].nIndex);
	}
}

void RestorePreservedSheets(CWorkbook& workbook)
{
	std::vector<CPreservedSheet> arSheets;
	arSheets.swap(workbook.m_arPreservedSheets);
	if (arSheets.empty())
		return;
	if (!workbook.m_oSheets.IsInit())
		workbook.m_oSheets.Init();

	std::vector<CSheet*>& arItems = workbook.m_oSheets->m_arrItems;
	size_t nEditorCount = arItems.size();
	std::stable_sort(arSheets.begin(), arSheets.end(), [](const CPreservedSheet& a, const CPreservedSheet& b) { return a.nIndex < b.nIndex; });

	// final order: >= 0 is an editor sheet index, < 0 is -(k + 1) for preserved sheet k
	std::vector<int> arOrder;
	for (size_t i = 0; i < nEditorCount; ++i)
		arOrder.push_back((int)i);
	for (size_t k = 0; k < arSheets.size(); ++k)
	{
		size_t nPos = std::min((size_t)std::max(arSheets[k].nIndex, 0), arOrder.size());
		arOrder.insert(arOrder.begin() + nPos, -(int)(k + 1));
	}
	std::vector<unsigned int> arEditorToFinal(nEditorCount), arPreservedToFinal(arSheets.size());
	for (size_t f = 0; f < arOrder.size(); ++f)
	{
		if (arOrder[f] >= 0)
			arEditorToFinal[arOrder[f]] = (unsigned int)f;
		else
			arPreservedToFinal[-arOrder[f] - 1] = (unsigned int)f;
	}

	std::set<std::wstring> setNames;
	unsigned int nMaxSheetId = 0;
	std::set<unsigned int> setSheetIds;
	for (CSheet* pSheet : arItems)
	{
		if (pSheet && pSheet->m_oName.IsInit())
			setNames.insert(ToLower(*pSheet->m_oName));
		if (pSheet && pSheet->m_oSheetId.IsInit())
		{
			setSheetIds.insert(pSheet->m_oSheetId->GetValue());
			nMaxSheetId = std::max(nMaxSheetId, pSheet->m_oSheetId->GetValue());
		}
	}
	for (const CPreservedSheet& oSheet : arSheets)
		nMaxSheetId = std::max(nMaxSheetId, oSheet.nSheetId);

	std::vector<CSheet*> arNewItems(arOrder.size(), NULL);
	for (size_t f = 0; f < arOrder.size(); ++f)
	{
		if (arOrder[f] >= 0)
		{
			arNewItems[f] = arItems[arOrder[f]];
			continue;
		}
		size_t k = -arOrder[f] - 1;
		const CPreservedSheet& oSheet = arSheets[k];

		std::wstring sName = oSheet.sName;
		for (int n = 2; setNames.end() != setNames.find(ToLower(sName)); ++n)
			sName = oSheet.sName + L" (" + std::to_wstring(n) + L")";
		setNames.insert(ToLower(sName));
		unsigned int nSheetId = oSheet.nSheetId;
		if (0 == nSheetId || setSheetIds.end() != setSheetIds.find(nSheetId))
			nSheetId = ++nMaxSheetId;
		setSheetIds.insert(nSheetId);

		smart_ptr<OOX::File> pFile(new CPreservedSheetFile(workbook.OOX::File::m_pMainDocument, oSheet, (int)k + 1));
		const OOX::RId oRId = workbook.Add(pFile);

		CSheet* pSheet = new CSheet();
		pSheet->m_oName = sName;
		pSheet->m_oSheetId.Init();
		pSheet->m_oSheetId->SetValue(nSheetId);
		if (!oSheet.sState.empty())
			{
				pSheet->m_oState.Init();
				pSheet->m_oState->FromString(oSheet.sState);
			}
		pSheet->m_oRid.Init();
		pSheet->m_oRid->SetValue(oRId.get());
		arNewItems[f] = pSheet;
	}
	arItems = arNewItems;

	auto remap = [&](unsigned int nEditorIndex) { return nEditorIndex < nEditorCount ? arEditorToFinal[nEditorIndex] : nEditorIndex; };
	if (workbook.m_oDefinedNames.IsInit())
	{
		for (CDefinedName* pName : workbook.m_oDefinedNames->m_arrItems)
		{
			if (pName && pName->m_oLocalSheetId.IsInit())
				pName->m_oLocalSheetId->SetValue(remap(pName->m_oLocalSheetId->GetValue()));
		}
	}
	for (size_t k = 0; k < arSheets.size(); ++k)
	{
		for (const CPreservedDefinedName& oName : arSheets[k].arDefinedNames)
		{
			XmlUtils::CXmlLiteReader oReader;
			if (!oReader.FromString(oName.sXml) || !oReader.ReadNextNode())
				continue;
			CDefinedName* pName = new CDefinedName(workbook.OOX::File::m_pMainDocument);
			pName->fromXML(oReader);
			if (oName.bLocal)
			{
				pName->m_oLocalSheetId.Init();
				pName->m_oLocalSheetId->SetValue(arPreservedToFinal[k]);
			}
			if (!workbook.m_oDefinedNames.IsInit())
				workbook.m_oDefinedNames.Init();
			workbook.m_oDefinedNames->m_arrItems.push_back(pName);
		}
	}
	if (workbook.m_oBookViews.IsInit())
	{
		for (CWorkbookView* pView : workbook.m_oBookViews->m_arrItems)
		{
			if (pView && pView->m_oActiveTab.IsInit())
				pView->m_oActiveTab->SetValue(remap(pView->m_oActiveTab->GetValue()));
			if (pView && pView->m_oFirstSheet.IsInit())
				pView->m_oFirstSheet->SetValue(remap(pView->m_oFirstSheet->GetValue()));
		}
	}
}
}
}
