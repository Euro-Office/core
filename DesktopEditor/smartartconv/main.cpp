/*
 * smartartconv — Bidirectional SmartArt PPTY binary ↔ OOXML XML converter
 *
 * Usage:
 *   smartartconv bin2xml  --data-dir <dir> --drawings <file> --out-dir <dir>
 *   smartartconv xml2bin  --xml-dir <dir> --out-data-dir <dir> --out-drawings <file>
 *   smartartconv verify   --data-dir <dir> --drawings <file> --tmp-dir <dir>
 *
 * (c) Copyright Euro-Office Contributors 2024-2026
 * AGPL-3.0 — see LICENSE for details
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>

#include "../common/File.h"
#include "../common/Directory.h"
#include "../common/Types.h"

#include "../../OOXML/Binary/Presentation/BinaryFileReaderWriter.h"
#include "../../OOXML/Binary/Presentation/XmlWriter.h"
#include "../../OOXML/Binary/Presentation/BinReaderWriterDefines.h"

#include "../../OOXML/DocxFormat/Diagram/DiagramData.h"
#include "../../OOXML/DocxFormat/Diagram/DiagramColors.h"
#include "../../OOXML/DocxFormat/Diagram/DiagramLayout.h"
#include "../../OOXML/DocxFormat/Diagram/DiagramQuickStyle.h"
#include "../../OOXML/DocxFormat/Diagram/DiagramDrawing.h"

// SmartArt type ID → filename mapping (from sdkjs/common/SmartArts/SmartArtCache.js
// and sdkjs/common/commonDefines.js)
static const std::map<int, std::wstring>& GetTypeToName()
{
	static const std::map<int, std::wstring> m = {
		{0,   L"Accented_Picture"},
		{1,   L"Balance"},
		{2,   L"Titled_Picture_Blocks"},
		{3,   L"Picture_Accent_Blocks"},
		{4,   L"Block_Cycle"},
		{5,   L"Stacked_Venn"},
		{6,   L"Vertical_Equation"},
		{7,   L"Vertical_Block_List"},
		{8,   L"Vertical_Bending_Process"},
		{9,   L"Vertical_Bullet_List"},
		{10,  L"Vertical_Curved_List"},
		{11,  L"Vertical_Process"},
		{12,  L"Vertical_Box_List"},
		{13,  L"Vertical_Picture_List"},
		{14,  L"Vertical_Circle_List"},
		{15,  L"Vertical_Picture_Accent_List"},
		{16,  L"Vertical_Arrow_List"},
		{17,  L"Vertical_Chevron_List"},
		{18,  L"Vertical_Accent_List"},
		{19,  L"Nested_Target"},
		{20,  L"Funnel"},
		{21,  L"Upward_Arrow"},
		{22,  L"Increasing_Arrows_Process"},
		{23,  L"Step_Up_Process"},
		{24,  L"Circular_Picture_Callout"},
		{25,  L"Horizontal_Hierarchy"},
		{26,  L"Horizontal_Labeled_Hierarchy"},
		{27,  L"Horizontal_Multi-Level_Hierarchy"},
		{28,  L"Horizontal_Organization_Chart"},
		{29,  L"Horizontal_Bullet_List"},
		{30,  L"Horizontal_Picture_List"},
		{31,  L"Closed_Chevron_Process"},
		{32,  L"Hierarchy_List"},
		{33,  L"Hierarchy"},
		{34,  L"Circle_Picture_Hierarchy"},
		{35,  L"Labeled_Hierarchy"},
		{36,  L"Inverted_Pyramid"},
		{37,  L"Hexagon_Cluster"},
		{38,  L"Circle_Relationship"},
		{39,  L"Circle_Accent_Timeline"},
		{40,  L"Circular_Bending_Process"},
		{41,  L"Arrow_Ribbon"},
		{42,  L"Linear_Venn"},
		{43,  L"Picture_Lineup"},
		{44,  L"Title_Picture_Lineup"},
		{45,  L"Bending_Picture_Caption_List"},
		{46,  L"Bending_Picture_Accent_List"},
		{47,  L"Titled_Matrix"},
		{48,  L"Increasing_Circle_Process"},
		{49,  L"Bending_Picture_Blocks"},
		{50,  L"Bending_Picture_Caption"},
		{51,  L"Bending_Picture_Semi-Transparent_Text"},
		{52,  L"Nondirectional_Cycle"},
		{53,  L"Continuous_Block_Process"},
		{54,  L"Continuous_Picture_List"},
		{55,  L"Continuous_Cycle"},
		{56,  L"Descending_Block_List"},
		{57,  L"Step_Down_Process"},
		{58,  L"Reverse_List"},
		{59,  L"Organization_Chart"},
		{60,  L"Name_and_Title_Organization_Chart"},
		{61,  L"Alternating_Flow"},
		{62,  L"Pyramid_List"},
		{63,  L"Plus_and_Minus"},
		{64,  L"Repeating_Bending_Process"},
		{65,  L"Captioned_Pictures"},
		{66,  L"Detailed_Process"},
		{67,  L"Picture_Strips"},
		{68,  L"Half_Circle_Organization_Chart"},
		{69,  L"Phased_Process"},
		{70,  L"Basic_Venn"},
		{71,  L"Basic_Timeline"},
		{72,  L"Basic_Pie"},
		{73,  L"Basic_Matrix"},
		{74,  L"Basic_Pyramid"},
		{75,  L"Basic_Radial"},
		{76,  L"Basic_Target"},
		{77,  L"Basic_Block_List"},
		{78,  L"Basic_Bending_Process"},
		{79,  L"Basic_Process"},
		{80,  L"Basic_Chevron_Process"},
		{81,  L"Basic_Cycle"},
		{82,  L"Opposing_Ideas"},
		{83,  L"Opposing_Arrows"},
		{84,  L"Random_to_Result_Process"},
		{85,  L"Sub-Step_Process"},
		{86,  L"Pie_Process"},
		{87,  L"Accent_Process"},
		{88,  L"Ascending_Picture_Accent_Process"},
		{89,  L"Picture_Accent_Process"},
		{90,  L"Radial_Venn"},
		{91,  L"Radial_Cycle"},
		{92,  L"Radial_Cluster"},
		{93,  L"Radial_List"},
		{94,  L"Multidirectional_Cycle"},
		{95,  L"Diverging_Radial_"},
		{96,  L"Diverging_Arrows"},
		{97,  L"Framed_Text_Picture"},
		{98,  L"Grouped_List"},
		{99,  L"Segmented_Pyramid"},
		{100, L"Segmented_Process"},
		{101, L"Segmented_Cycle"},
		{102, L"Picture_Grid"},
		{103, L"Grid_Matrix"},
		{104, L"Spiral_Picture"},
		{105, L"Stacked_List"},
		{106, L"Picture_Caption_List"},
		{107, L"Process_List"},
		{108, L"Bubble_Picture_List"},
		{109, L"Square_Accent_List"},
		{110, L"Lined_List"},
		{111, L"Picture_Accent_List"},
		{112, L"Titled_Picture_Accent_List"},
		{113, L"Snapshot_Picture_List"},
		{114, L"Continuous_Arrow_Process"},
		{115, L"Circle_Arrow_Process"},
		{116, L"Process_Arrows"},
		{117, L"Staggered_Process"},
		{118, L"Converging_Radial"},
		{119, L"Converging_Arrows"},
		{120, L"Table_Hierarchy"},
		{121, L"Table_List"},
		{122, L"Text_Cycle"},
		{123, L"Trapezoid_List"},
		{124, L"Descending_Process"},
		{125, L"Chevron_List"},
		{126, L"Equation"},
		{127, L"Counterbalance_Arrows"},
		{128, L"Target_List"},
		{129, L"Cycle_Matrix"},
		{130, L"Alternating_Picture_Blocks"},
		{131, L"Alternating_Picture_Circles"},
		{132, L"Alternating_Hexagons"},
		{133, L"Gear"},
		{134, L"Architecture_Layout"},
		{135, L"Chevron_Accent_Process"},
		{136, L"Circle_Process"},
		{137, L"Converging_Text"},
		{138, L"Hexagon_Radial"},
		{139, L"Interconnected_Block_Process"},
		{140, L"Interconnected_Rings"},
		{141, L"Picture_Frame"},
		{142, L"Picture_Organization_Chart"},
		{143, L"Radial_Picture_List"},
		{144, L"Tab_List"},
		{145, L"Tabbed_Arc"},
		{146, L"Theme_Picture_Accent"},
		{147, L"Theme_Picture_Alternating_Accent"},
		{148, L"Theme_Picture_Grid"},
		{149, L"Varying_Width_List"},
		{150, L"Vertical_Bracket_List"},
	};
	return m;
}

static std::map<std::wstring, int> GetNameToType()
{
	std::map<std::wstring, int> result;
	for (auto& p : GetTypeToName())
		result[p.second] = p.first;
	return result;
}

// --- Helpers ---

static std::wstring Utf8ToWide(const std::string& s)
{
	std::wstring result;
	result.reserve(s.size());
	for (size_t i = 0; i < s.size(); )
	{
		unsigned char c = s[i];
		wchar_t ch;
		if (c < 0x80) { ch = c; i += 1; }
		else if (c < 0xE0) { ch = ((c & 0x1F) << 6) | (s[i+1] & 0x3F); i += 2; }
		else if (c < 0xF0) { ch = ((c & 0x0F) << 12) | ((s[i+1] & 0x3F) << 6) | (s[i+2] & 0x3F); i += 3; }
		else { ch = L'?'; i += 4; }
		result += ch;
	}
	return result;
}

static std::string WideToUtf8(const std::wstring& ws)
{
	std::string result;
	for (wchar_t ch : ws)
	{
		if (ch < 0x80) result += (char)ch;
		else if (ch < 0x800) { result += (char)(0xC0 | (ch >> 6)); result += (char)(0x80 | (ch & 0x3F)); }
		else { result += (char)(0xE0 | (ch >> 12)); result += (char)(0x80 | ((ch >> 6) & 0x3F)); result += (char)(0x80 | (ch & 0x3F)); }
	}
	return result;
}

static void WriteXmlToFile(const std::wstring& path, NSBinPptxRW::CXmlWriter& xmlWriter)
{
	NSFile::CFileBinary::SaveToFile(path, xmlWriter.GetXmlString());
}

template<typename T>
static void DiagramObjToXml(NSBinPptxRW::CXmlWriter& xmlWriter, T& obj)
{
	xmlWriter.WriteString(L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
	xmlWriter.m_lDocType = XMLWRITER_DOC_TYPE_DIAGRAM;
	obj.toXmlWriter(&xmlWriter);
}

// --- bin2xml: Convert a single SmartArtData .bin to 4 XML files ---

static bool ConvertDataBinToXml(const std::wstring& binPath, const std::wstring& outDir)
{
	BYTE* pData = nullptr;
	DWORD nSize = 0;
	if (!NSFile::CFileBinary::ReadAllBytes(binPath, &pData, nSize))
	{
		std::cerr << "  ERROR: Cannot read " << WideToUtf8(binPath) << std::endl;
		return false;
	}

	NSBinPptxRW::CBinaryFileReader reader;
	reader.Init(pData, 0, (int)nSize);
	reader.m_nDocumentType = XMLWRITER_DOC_TYPE_DIAGRAM;

	NSDirectory::CreateDirectory(outDir);

	int recordCount = 0;
	while (reader.GetPos() < (LONG)nSize && recordCount < 4)
	{
		BYTE recType = reader.GetUChar();

		switch (recType)
		{
			case 1:
			{
				OOX::CDiagramData obj(nullptr);
				obj.fromPPTY(&reader);
				NSBinPptxRW::CXmlWriter xw;
				DiagramObjToXml(xw, obj);
				WriteXmlToFile(outDir + L"/data.xml", xw);
			} break;
			case 2:
			{
				OOX::CDiagramColors obj(nullptr);
				obj.fromPPTY(&reader);
				NSBinPptxRW::CXmlWriter xw;
				DiagramObjToXml(xw, obj);
				WriteXmlToFile(outDir + L"/colors.xml", xw);
			} break;
			case 3:
			{
				OOX::CDiagramLayout obj(nullptr);
				obj.fromPPTY(&reader);
				NSBinPptxRW::CXmlWriter xw;
				DiagramObjToXml(xw, obj);
				WriteXmlToFile(outDir + L"/layout.xml", xw);
			} break;
			case 4:
			{
				OOX::CDiagramQuickStyle obj(nullptr);
				obj.fromPPTY(&reader);
				NSBinPptxRW::CXmlWriter xw;
				DiagramObjToXml(xw, obj);
				WriteXmlToFile(outDir + L"/quickstyle.xml", xw);
			} break;
			default:
			{
				reader.SkipRecord();
			} break;
		}
		recordCount++;
	}

	delete[] pData;
	return true;
}

// --- bin2xml: Convert SmartArtDrawings.bin → individual drawing XML files ---

static bool ConvertDrawingsBinToXml(const std::wstring& binPath, const std::wstring& outDir)
{
	BYTE* pData = nullptr;
	DWORD nSize = 0;
	if (!NSFile::CFileBinary::ReadAllBytes(binPath, &pData, nSize))
	{
		std::cerr << "  ERROR: Cannot read " << WideToUtf8(binPath) << std::endl;
		return false;
	}

	NSDirectory::CreateDirectory(outDir);

	// Parse consolidated format:
	// [UChar marker][ULong blobLength][blob...][UChar marker][ULong indexLength][(UChar type, ULong offset)...]
	size_t pos = 0;
	if (pos >= nSize) { delete[] pData; return false; }

	pos++; // skip marker byte
	if (pos + 4 > nSize) { delete[] pData; return false; }

	_UINT32 blobLength = *((_UINT32*)(pData + pos));
	pos += 4;

	BYTE* pBlob = pData + pos;
	DWORD blobSize = blobLength;
	pos += blobLength;

	if (pos >= nSize) { delete[] pData; return false; }
	pos++; // skip marker byte

	if (pos + 4 > nSize) { delete[] pData; return false; }
	_UINT32 indexLength = *((_UINT32*)(pData + pos));
	pos += 4;

	size_t indexEnd = pos + indexLength;

	// Read index entries
	std::map<int, _UINT32> typeOffsets;
	while (pos < indexEnd && pos < nSize)
	{
		BYTE type = pData[pos]; pos++;
		if (pos + 4 > nSize) break;
		_UINT32 offset = *((_UINT32*)(pData + pos)); pos += 4;
		typeOffsets[type] = offset;
	}

	const auto& typeToName = GetTypeToName();

	for (auto& entry : typeOffsets)
	{
		int smartArtType = entry.first;
		_UINT32 offset = entry.second;

		auto it = typeToName.find(smartArtType);
		if (it == typeToName.end()) continue;

		NSBinPptxRW::CBinaryFileReader reader;
		reader.Init(pBlob, 0, (int)blobSize);
		reader.Seek(offset);
		reader.m_nDocumentType = XMLWRITER_DOC_TYPE_DIAGRAM;

		BYTE recType = reader.GetUChar();
		(void)recType; // should be 0 (drawing)

		OOX::CDiagramDrawing obj(nullptr);
		obj.fromPPTY(&reader);

		NSBinPptxRW::CXmlWriter xw;
		xw.WriteString(L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
		xw.m_lDocType = XMLWRITER_DOC_TYPE_DSP_DRAWING;
		obj.toXmlWriter(&xw);

		WriteXmlToFile(outDir + L"/" + it->second + L".xml", xw);
	}

	delete[] pData;
	return true;
}

// --- xml2bin: Convert XML data directory → single .bin ---

static bool ConvertXmlToDataBin(const std::wstring& xmlDir, const std::wstring& outBinPath)
{
	NSBinPptxRW::CBinaryFileWriter writer;

	// Record 1: DiagramData
	{
		std::wstring xmlPath = xmlDir + L"/data.xml";
		if (NSFile::CFileBinary::Exists(xmlPath))
		{
			OOX::CDiagramData obj(nullptr);
			obj.read(OOX::CPath(xmlPath));
			writer.StartRecord(1);
			obj.toPPTY(&writer);
			writer.EndRecord();
		}
	}
	// Record 2: DiagramColors
	{
		std::wstring xmlPath = xmlDir + L"/colors.xml";
		if (NSFile::CFileBinary::Exists(xmlPath))
		{
			OOX::CDiagramColors obj(nullptr);
			obj.read(OOX::CPath(xmlPath));
			writer.StartRecord(2);
			obj.toPPTY(&writer);
			writer.EndRecord();
		}
	}
	// Record 3: DiagramLayout
	{
		std::wstring xmlPath = xmlDir + L"/layout.xml";
		if (NSFile::CFileBinary::Exists(xmlPath))
		{
			OOX::CDiagramLayout obj(nullptr);
			obj.read(OOX::CPath(xmlPath));
			writer.StartRecord(3);
			obj.toPPTY(&writer);
			writer.EndRecord();
		}
	}
	// Record 4: DiagramQuickStyle
	{
		std::wstring xmlPath = xmlDir + L"/quickstyle.xml";
		if (NSFile::CFileBinary::Exists(xmlPath))
		{
			OOX::CDiagramQuickStyle obj(nullptr);
			obj.read(OOX::CPath(xmlPath));
			writer.StartRecord(4);
			obj.toPPTY(&writer);
			writer.EndRecord();
		}
	}

	// Save binary
	NSFile::CFileBinary oFile;
	if (oFile.CreateFileW(outBinPath))
	{
		oFile.WriteFile(writer.GetBuffer(), writer.GetPosition());
		oFile.CloseFile();
		return true;
	}
	return false;
}

// --- xml2bin: Convert drawing XML files → consolidated SmartArtDrawings.bin ---

static bool ConvertXmlToDrawingsBin(const std::wstring& xmlDir, const std::wstring& outBinPath)
{
	const auto& typeToName = GetTypeToName();

	// Phase 1: Build the blob — serialize all drawings into one buffer
	NSBinPptxRW::CBinaryFileWriter blobWriter;
	std::map<int, _UINT32> typeOffsets;

	for (auto& entry : typeToName)
	{
		int smartArtType = entry.first;
		std::wstring xmlPath = xmlDir + L"/" + entry.second + L".xml";

		if (!NSFile::CFileBinary::Exists(xmlPath))
			continue;

		_UINT32 offset = blobWriter.GetPosition();
		typeOffsets[smartArtType] = offset;

		OOX::CDiagramDrawing obj(nullptr);
		obj.read(OOX::CPath(xmlPath));

		blobWriter.StartRecord(0); // type 0 = drawing
		obj.toPPTY(&blobWriter);
		blobWriter.EndRecord();
	}

	// Phase 2: Write consolidated file
	NSFile::CFileBinary oFile;
	if (!oFile.CreateFileW(outBinPath))
		return false;

	// Blob section: [UChar 0x00][ULong blobLength][blob...]
	BYTE marker = 0x00;
	oFile.WriteFile(&marker, 1);

	_UINT32 blobLength = blobWriter.GetPosition();
	oFile.WriteFile((BYTE*)&blobLength, 4);
	oFile.WriteFile(blobWriter.GetBuffer(), blobLength);

	// Index section: [UChar 0x00][ULong indexLength][(UChar, ULong)...]
	oFile.WriteFile(&marker, 1);

	_UINT32 indexLength = (_UINT32)(typeOffsets.size() * 5); // 1 + 4 bytes per entry
	oFile.WriteFile((BYTE*)&indexLength, 4);

	for (auto& entry : typeOffsets)
	{
		BYTE type = (BYTE)entry.first;
		_UINT32 offset = entry.second;
		oFile.WriteFile(&type, 1);
		oFile.WriteFile((BYTE*)&offset, 4);
	}

	oFile.CloseFile();
	return true;
}

// --- verify: Round-trip test ---

static bool FilesEqual(const std::wstring& path1, const std::wstring& path2)
{
	BYTE* pData1 = nullptr; DWORD nSize1 = 0;
	BYTE* pData2 = nullptr; DWORD nSize2 = 0;

	if (!NSFile::CFileBinary::ReadAllBytes(path1, &pData1, nSize1)) return false;
	if (!NSFile::CFileBinary::ReadAllBytes(path2, &pData2, nSize2))
	{
		delete[] pData1;
		return false;
	}

	bool equal = (nSize1 == nSize2) && (memcmp(pData1, pData2, nSize1) == 0);
	delete[] pData1;
	delete[] pData2;
	return equal;
}

static int RunVerify(const std::wstring& dataDir, const std::wstring& drawingsPath, const std::wstring& tmpDir)
{
	NSDirectory::CreateDirectory(tmpDir);

	std::wstring xmlDir = tmpDir + L"/xml";
	std::wstring xmlDataDir = xmlDir + L"/data";
	std::wstring xmlDrawingsDir = xmlDir + L"/drawings";
	std::wstring binDir = tmpDir + L"/bin";
	std::wstring binDataDir = binDir + L"/data";
	std::wstring binDrawingsPath = binDir + L"/SmartArtDrawings.bin";

	NSDirectory::CreateDirectory(xmlDir);
	NSDirectory::CreateDirectory(xmlDataDir);
	NSDirectory::CreateDirectory(xmlDrawingsDir);
	NSDirectory::CreateDirectory(binDir);
	NSDirectory::CreateDirectory(binDataDir);

	const auto& typeToName = GetTypeToName();
	int passed = 0, failed = 0, skipped = 0;

	std::cout << "=== SmartArt Round-Trip Verification ===" << std::endl;

	// Test data .bin files
	for (auto& entry : typeToName)
	{
		const std::wstring& name = entry.second;
		std::wstring srcBin = dataDir + L"/" + name + L".bin";

		if (!NSFile::CFileBinary::Exists(srcBin))
		{
			skipped++;
			continue;
		}

		std::wstring xmlOut = xmlDataDir + L"/" + name;
		std::wstring dstBin = binDataDir + L"/" + name + L".bin";

		// bin → xml
		if (!ConvertDataBinToXml(srcBin, xmlOut))
		{
			std::cerr << "  FAIL (bin2xml): " << WideToUtf8(name) << std::endl;
			failed++;
			continue;
		}

		// xml → bin
		if (!ConvertXmlToDataBin(xmlOut, dstBin))
		{
			std::cerr << "  FAIL (xml2bin): " << WideToUtf8(name) << std::endl;
			failed++;
			continue;
		}

		// Compare
		if (FilesEqual(srcBin, dstBin))
		{
			passed++;
		}
		else
		{
			// Not byte-identical — try XML comparison (bin → xml again from regenerated)
			std::wstring xmlOut2 = xmlDataDir + L"/" + name + L"_rt";
			ConvertDataBinToXml(dstBin, xmlOut2);

			bool xmlMatch = true;
			for (auto& xmlFile : {L"/data.xml", L"/colors.xml", L"/layout.xml", L"/quickstyle.xml"})
			{
				std::wstring f1 = xmlOut + xmlFile;
				std::wstring f2 = xmlOut2 + xmlFile;
				if (NSFile::CFileBinary::Exists(f1) && NSFile::CFileBinary::Exists(f2))
				{
					if (!FilesEqual(f1, f2))
					{
						xmlMatch = false;
						break;
					}
				}
			}

			if (xmlMatch)
			{
				std::cout << "  PASS (xml-equiv): " << WideToUtf8(name) << std::endl;
				passed++;
			}
			else
			{
				std::cerr << "  FAIL: " << WideToUtf8(name) << " (binary and XML differ)" << std::endl;
				failed++;
			}
		}
	}

	// Test drawings
	if (NSFile::CFileBinary::Exists(drawingsPath))
	{
		std::cout << "Testing SmartArtDrawings.bin..." << std::endl;
		ConvertDrawingsBinToXml(drawingsPath, xmlDrawingsDir);
		ConvertXmlToDrawingsBin(xmlDrawingsDir, binDrawingsPath);

		if (FilesEqual(drawingsPath, binDrawingsPath))
		{
			std::cout << "  PASS: SmartArtDrawings.bin" << std::endl;
			passed++;
		}
		else
		{
			// XML-level comparison
			std::wstring xmlDrawingsDir2 = xmlDir + L"/drawings_rt";
			NSDirectory::CreateDirectory(xmlDrawingsDir2);
			ConvertDrawingsBinToXml(binDrawingsPath, xmlDrawingsDir2);

			bool allMatch = true;
			for (auto& entry : typeToName)
			{
				std::wstring f1 = xmlDrawingsDir + L"/" + entry.second + L".xml";
				std::wstring f2 = xmlDrawingsDir2 + L"/" + entry.second + L".xml";
				if (NSFile::CFileBinary::Exists(f1) && NSFile::CFileBinary::Exists(f2))
				{
					if (!FilesEqual(f1, f2))
					{
						allMatch = false;
						std::cerr << "  FAIL drawing: " << WideToUtf8(entry.second) << std::endl;
					}
				}
			}

			if (allMatch)
			{
				std::cout << "  PASS (xml-equiv): SmartArtDrawings.bin" << std::endl;
				passed++;
			}
			else
			{
				failed++;
			}
		}
	}

	std::cout << std::endl;
	std::cout << "Results: " << passed << " passed, " << failed << " failed, " << skipped << " skipped" << std::endl;
	return failed > 0 ? 1 : 0;
}

// --- CLI argument parsing ---

static std::string GetArg(int argc, char* argv[], const std::string& flag)
{
	for (int i = 0; i < argc - 1; i++)
	{
		if (flag == argv[i])
			return argv[i + 1];
	}
	return "";
}

static void PrintUsage()
{
	std::cout << "Usage:" << std::endl;
	std::cout << "  smartartconv bin2xml  --data-dir <dir> --drawings <file> --out-dir <dir>" << std::endl;
	std::cout << "  smartartconv xml2bin  --xml-dir <dir> --out-data-dir <dir> --out-drawings <file>" << std::endl;
	std::cout << "  smartartconv verify   --data-dir <dir> --drawings <file> --tmp-dir <dir>" << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		PrintUsage();
		return 1;
	}

	std::string mode = argv[1];
	const auto& typeToName = GetTypeToName();

	if (mode == "bin2xml")
	{
		std::wstring dataDir = Utf8ToWide(GetArg(argc, argv, "--data-dir"));
		std::wstring drawings = Utf8ToWide(GetArg(argc, argv, "--drawings"));
		std::wstring outDir = Utf8ToWide(GetArg(argc, argv, "--out-dir"));

		if (outDir.empty())
		{
			std::cerr << "Missing --out-dir" << std::endl;
			return 1;
		}

		NSDirectory::CreateDirectory(outDir);
		std::wstring outDataDir = outDir + L"/data";
		std::wstring outDrawingsDir = outDir + L"/drawings";
		NSDirectory::CreateDirectory(outDataDir);
		NSDirectory::CreateDirectory(outDrawingsDir);

		int converted = 0;
		if (!dataDir.empty())
		{
			for (auto& entry : typeToName)
			{
				std::wstring binPath = dataDir + L"/" + entry.second + L".bin";
				if (!NSFile::CFileBinary::Exists(binPath))
					continue;

				std::wstring xmlOutDir = outDataDir + L"/" + entry.second;
				std::cout << "Converting " << WideToUtf8(entry.second) << "..." << std::endl;
				if (ConvertDataBinToXml(binPath, xmlOutDir))
					converted++;
			}
			std::cout << "Converted " << converted << " data files." << std::endl;
		}

		if (!drawings.empty() && NSFile::CFileBinary::Exists(drawings))
		{
			std::cout << "Converting drawings..." << std::endl;
			ConvertDrawingsBinToXml(drawings, outDrawingsDir);
			std::cout << "Done." << std::endl;
		}

		return 0;
	}
	else if (mode == "xml2bin")
	{
		std::wstring xmlDir = Utf8ToWide(GetArg(argc, argv, "--xml-dir"));
		std::wstring outDataDir = Utf8ToWide(GetArg(argc, argv, "--out-data-dir"));
		std::wstring outDrawings = Utf8ToWide(GetArg(argc, argv, "--out-drawings"));

		if (xmlDir.empty())
		{
			std::cerr << "Missing --xml-dir" << std::endl;
			return 1;
		}

		int converted = 0;
		if (!outDataDir.empty())
		{
			NSDirectory::CreateDirectory(outDataDir);

			for (auto& entry : typeToName)
			{
				std::wstring xmlSubDir = xmlDir + L"/data/" + entry.second;
				if (!NSFile::CFileBinary::Exists(xmlSubDir + L"/layout.xml"))
					continue;

				std::wstring binPath = outDataDir + L"/" + entry.second + L".bin";
				std::cout << "Building " << WideToUtf8(entry.second) << ".bin..." << std::endl;
				if (ConvertXmlToDataBin(xmlSubDir, binPath))
					converted++;
			}
			std::cout << "Built " << converted << " data files." << std::endl;
		}

		if (!outDrawings.empty())
		{
			std::wstring xmlDrawingsDir = xmlDir + L"/drawings";
			std::cout << "Building SmartArtDrawings.bin..." << std::endl;

			// Ensure parent directory exists
			std::wstring parentDir = outDrawings.substr(0, outDrawings.rfind(L'/'));
			if (!parentDir.empty())
				NSDirectory::CreateDirectory(parentDir);

			ConvertXmlToDrawingsBin(xmlDrawingsDir, outDrawings);
			std::cout << "Done." << std::endl;
		}

		return 0;
	}
	else if (mode == "verify")
	{
		std::wstring dataDir = Utf8ToWide(GetArg(argc, argv, "--data-dir"));
		std::wstring drawings = Utf8ToWide(GetArg(argc, argv, "--drawings"));
		std::wstring tmpDir = Utf8ToWide(GetArg(argc, argv, "--tmp-dir"));

		if (dataDir.empty() || tmpDir.empty())
		{
			std::cerr << "Missing --data-dir or --tmp-dir" << std::endl;
			return 1;
		}

		return RunVerify(dataDir, drawings, tmpDir);
	}
	else
	{
		std::cerr << "Unknown mode: " << mode << std::endl;
		PrintUsage();
		return 1;
	}
}
