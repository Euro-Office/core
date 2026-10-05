#pragma once

#include <string>
#include <vector>
#include "../../../Base/Base.h"
#include <boost/shared_ptr.hpp>
#include "../../../../DesktopEditor/common/base_export.h"

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

class Q_DECL_EXPORT DatabaseReader
{
public:
	DatabaseReader();
	~DatabaseReader();

	_UINT32 Read(const std::wstring &sFileName, OOX::Spreadsheet::CXlsx &oXlsx, _INT32 lcid, bool readToCache);
};
