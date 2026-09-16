#pragma once

#include <string>
#include <vector>
#include "../../../Base/Base.h"
#include <boost/shared_ptr.hpp>

namespace OOX {
	namespace Spreadsheet {
		class CXlsx;
	}
}

namespace NExtractTools
{
	class InputParams;
	class ConvertParams;

	_UINT32 db2xlsx_dir(const std::wstring& sFrom, const std::wstring& sTo, InputParams& params, ConvertParams& convertParams);
}

class DatabaseReader
{
public:
	DatabaseReader();
	~DatabaseReader();

	_UINT32 Read(const std::wstring &sFileName, OOX::Spreadsheet::CXlsx &oXlsx, _INT32 lcid, bool readToCache);
};
