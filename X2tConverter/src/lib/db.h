#include "../../../../OOXML/Binary/Sheets/Common/Common.h"
#include "../../../../OOXML/Binary/Sheets/Reader/DatabaseReader.h"
#include "../../../../OOXML/XlsxFormat/Xlsx.h"
#include "common.h"

namespace NExtractTools
{
	_UINT32 db2xlsx_dir(const std::wstring& sFrom, const std::wstring& sTo, InputParams& params, ConvertParams& convertParams)
	{
		params.m_bMacro = false;

		OOX::Spreadsheet::CXlsx oXlsx;
        _INT32 lcid = -1;

		DatabaseReader dbReader;
        _UINT32 nRes = dbReader.Read(sFrom, oXlsx, lcid, false);

		if (SUCCEEDED_X2T(nRes))
		{
			oXlsx.PrepareToWrite();

			OOX::CContentTypes oContentTypes;
			nRes = oXlsx.Write(sTo, oContentTypes) ? nRes : AVS_FILEUTILS_ERROR_CONVERT;
		}
		return nRes;
	}
}
