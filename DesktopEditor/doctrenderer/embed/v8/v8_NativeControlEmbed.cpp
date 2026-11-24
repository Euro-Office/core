// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../NativeControlEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSNativeControlEmbed
{
#define CURRENTWRAPPER CNativeControlEmbed

	FUNCTION_WRAPPER_V8_1(SetFilePathWrapper, SetFilePath)
	FUNCTION_WRAPPER_V8_0(GetFilePathWrapper, GetFilePath)
	FUNCTION_WRAPPER_V8_1(SetFileIdWrapper, SetFileId)
	FUNCTION_WRAPPER_V8_0(GetFileIdWrapper, GetFileId)
	FUNCTION_WRAPPER_V8_1(GetFileBinaryWrapper, GetFileBinary)
	FUNCTION_WRAPPER_V8_1(GetFontBinaryWrapper, GetFontBinary)
	FUNCTION_WRAPPER_V8_0(GetFontsDirectoryWrapper, GetFontsDirectory)
	FUNCTION_WRAPPER_V8_1(GetFileStringWrapper, GetFileString)
	FUNCTION_WRAPPER_V8_0(GetEditorTypeWrapper, GetEditorType)
	FUNCTION_WRAPPER_V8_0(CheckNextChangeWrapper, CheckNextChange)
	FUNCTION_WRAPPER_V8_0(GetCountChangesWrapper, GetCountChanges)
	FUNCTION_WRAPPER_V8_1(GetChangesFileWrapper, GetChangesFile)
	FUNCTION_WRAPPER_V8_2(Save_EndWrapper, Save_End)
	FUNCTION_WRAPPER_V8_1(AddImageInChangesWrapper, AddImageInChanges)
	FUNCTION_WRAPPER_V8_1(ConsoleLogWrapper, ConsoleLog)
	FUNCTION_WRAPPER_V8_1(ZipOpenWrapper, ZipOpen)
	FUNCTION_WRAPPER_V8_1(ZipOpenBase64Wrapper, ZipOpenBase64)
	FUNCTION_WRAPPER_V8_1(ZipFileAsStringWrapper, ZipFileAsString)
	FUNCTION_WRAPPER_V8_1(ZipFileAsBinaryWrapper, ZipFileAsBinary)
	FUNCTION_WRAPPER_V8_0(ZipCloseWrapper, ZipClose)
	FUNCTION_WRAPPER_V8_1(GetImageUrlWrapper, GetImageUrl)
	FUNCTION_WRAPPER_V8_0(GetImagesPathWrapper, GetImagesPath)
	FUNCTION_WRAPPER_V8_1(GetImageOriginalSizeWrapper, GetImageOriginalSize)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "SetFilePath", SetFilePathWrapper);
		NSV8Objects::Template_Set(result, "GetFilePath", GetFilePathWrapper);
		NSV8Objects::Template_Set(result, "SetFileId", SetFileIdWrapper);
		NSV8Objects::Template_Set(result, "GetFileId", GetFileIdWrapper);
		NSV8Objects::Template_Set(result, "GetFileBinary", GetFileBinaryWrapper);
		NSV8Objects::Template_Set(result, "GetFontBinary", GetFontBinaryWrapper);
		NSV8Objects::Template_Set(result, "GetFontsDirectory", GetFontsDirectoryWrapper);
		NSV8Objects::Template_Set(result, "GetFileString", GetFileStringWrapper);
		NSV8Objects::Template_Set(result, "GetEditorType", GetEditorTypeWrapper);
		NSV8Objects::Template_Set(result, "CheckNextChange", CheckNextChangeWrapper);
		NSV8Objects::Template_Set(result, "GetCountChanges", GetCountChangesWrapper);
		NSV8Objects::Template_Set(result, "GetChangesFile", GetChangesFileWrapper);
		NSV8Objects::Template_Set(result, "Save_End", Save_EndWrapper);
		NSV8Objects::Template_Set(result, "AddImageInChanges", AddImageInChangesWrapper);
		NSV8Objects::Template_Set(result, "ConsoleLog", ConsoleLogWrapper);
		NSV8Objects::Template_Set(result, "ZipOpen", ZipOpenWrapper);
		NSV8Objects::Template_Set(result, "ZipOpenBase64", ZipOpenBase64Wrapper);
		NSV8Objects::Template_Set(result, "ZipFileAsString", ZipFileAsStringWrapper);
		NSV8Objects::Template_Set(result, "ZipFileAsBinary", ZipFileAsBinaryWrapper);
		NSV8Objects::Template_Set(result, "ZipClose", ZipCloseWrapper);
		NSV8Objects::Template_Set(result, "GetImageUrl", GetImageUrlWrapper);
		NSV8Objects::Template_Set(result, "GetImagesPath", GetImagesPathWrapper);
		NSV8Objects::Template_Set(result, "GetImageOriginalSize", GetImageOriginalSizeWrapper);

		return handle_scope.Escape(result);
	}
}

class CNativeControlEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSNativeControlEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CNativeControlEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CNativeControlEmbedAdapter();
	return m_pAdapter;
}

std::string CNativeControlEmbed::getName() { return "CNativeControlEmbed"; }

CJSEmbedObject* CNativeControlEmbed::getCreator()
{
	return new CNativeControlEmbed();
}
