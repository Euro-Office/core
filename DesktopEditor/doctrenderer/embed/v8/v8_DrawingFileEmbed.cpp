// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../DrawingFileEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSDrawingFileEmbed
{
#define CURRENTWRAPPER CDrawingFileEmbed

	FUNCTION_WRAPPER_V8_2(OpenFileWrapper, OpenFile)
	FUNCTION_WRAPPER_V8_0(CloseFileWrapper, CloseFile)
	FUNCTION_WRAPPER_V8_0(GetTypeWrapper, GetType)
	FUNCTION_WRAPPER_V8_0(GetErrorCodeWrapper, GetErrorCode)
	FUNCTION_WRAPPER_V8_0(GetInfoWrapper, GetInfo)
	FUNCTION_WRAPPER_V8_4(GetPixmapWrapper, GetPixmap)
	FUNCTION_WRAPPER_V8_1(DestroyPixmapWrapper, DestroyPixmap)
	FUNCTION_WRAPPER_V8_1(GetLinksWrapper, GetLinks)
	FUNCTION_WRAPPER_V8_0(GetStructureWrapper, GetStructure)
	FUNCTION_WRAPPER_V8_0(GetInteractiveFormsInfoWrapper, GetInteractiveFormsInfo)
	FUNCTION_WRAPPER_V8_1(GetInteractiveFormsFontsWrapper, GetInteractiveFormsFonts)
	FUNCTION_WRAPPER_V8_7(GetInteractiveFormsAPWrapper, GetInteractiveFormsAP)
	FUNCTION_WRAPPER_V8_5(GetButtonIconsWrapper, GetButtonIcons)
	FUNCTION_WRAPPER_V8_1(GetAnnotationsInfoWrapper, GetAnnotationsInfo)
	FUNCTION_WRAPPER_V8_6(GetAnnotationsAPWrapper, GetAnnotationsAP)
	FUNCTION_WRAPPER_V8_1(GetFontBinaryWrapper, GetFontBinary)
	FUNCTION_WRAPPER_V8_1(GetGlyphsWrapper, GetGlyphs)
	FUNCTION_WRAPPER_V8_0(DestroyTextInfoWrapper, DestroyTextInfo)
	FUNCTION_WRAPPER_V8_1(GetGIDByUnicodeWrapper, GetGIDByUnicode)
	FUNCTION_WRAPPER_V8_0(IsNeedCMapWrapper, IsNeedCMap)
	FUNCTION_WRAPPER_V8_2(ScanPageWrapper, ScanPage)
	FUNCTION_WRAPPER_V8_1(SetScanPageFontsWrapper, SetScanPageFonts)
	FUNCTION_WRAPPER_V8_1(GetImageBase64Wrapper, GetImageBase64)
	FUNCTION_WRAPPER_V8_1(FreeWasmDataWrapper, FreeWasmData)
	FUNCTION_WRAPPER_V8_2(SplitPagesWrapper, SplitPages)
	FUNCTION_WRAPPER_V8_3(MergePagesWrapper, MergePages)
	FUNCTION_WRAPPER_V8_0(UnmergePagesWrapper, UnmergePages)
	FUNCTION_WRAPPER_V8_3(RedactPageWrapper, RedactPage)
	FUNCTION_WRAPPER_V8_0(UndoRedactWrapper, UndoRedact)
	FUNCTION_WRAPPER_V8_1(CheckOwnerPasswordWrapper, CheckOwnerPassword)
	FUNCTION_WRAPPER_V8_1(CheckPermWrapper, CheckPerm)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "OpenFile", OpenFileWrapper);
		NSV8Objects::Template_Set(result, "CloseFile", CloseFileWrapper);
		NSV8Objects::Template_Set(result, "GetType", GetTypeWrapper);
		NSV8Objects::Template_Set(result, "GetErrorCode", GetErrorCodeWrapper);
		NSV8Objects::Template_Set(result, "GetInfo", GetInfoWrapper);
		NSV8Objects::Template_Set(result, "GetPixmap", GetPixmapWrapper);
		NSV8Objects::Template_Set(result, "DestroyPixmap", DestroyPixmapWrapper);
		NSV8Objects::Template_Set(result, "GetLinks", GetLinksWrapper);
		NSV8Objects::Template_Set(result, "GetStructure", GetStructureWrapper);
		NSV8Objects::Template_Set(result, "GetInteractiveFormsInfo", GetInteractiveFormsInfoWrapper);
		NSV8Objects::Template_Set(result, "GetInteractiveFormsFonts", GetInteractiveFormsFontsWrapper);
		NSV8Objects::Template_Set(result, "GetInteractiveFormsAP", GetInteractiveFormsAPWrapper);
		NSV8Objects::Template_Set(result, "GetButtonIcons", GetButtonIconsWrapper);
		NSV8Objects::Template_Set(result, "GetAnnotationsInfo", GetAnnotationsInfoWrapper);
		NSV8Objects::Template_Set(result, "GetAnnotationsAP", GetAnnotationsAPWrapper);
		NSV8Objects::Template_Set(result, "GetFontBinary", GetFontBinaryWrapper);
		NSV8Objects::Template_Set(result, "GetGlyphs", GetGlyphsWrapper);
		NSV8Objects::Template_Set(result, "DestroyTextInfo", DestroyTextInfoWrapper);
		NSV8Objects::Template_Set(result, "GetGIDByUnicode", GetGIDByUnicodeWrapper);
		NSV8Objects::Template_Set(result, "IsNeedCMap", IsNeedCMapWrapper);
		NSV8Objects::Template_Set(result, "ScanPage", ScanPageWrapper);
		NSV8Objects::Template_Set(result, "SetScanPageFonts", SetScanPageFontsWrapper);
		NSV8Objects::Template_Set(result, "GetImageBase64", GetImageBase64Wrapper);
		NSV8Objects::Template_Set(result, "FreeWasmData", FreeWasmDataWrapper);
		NSV8Objects::Template_Set(result, "SplitPages", SplitPagesWrapper);
		NSV8Objects::Template_Set(result, "MergePages", MergePagesWrapper);
		NSV8Objects::Template_Set(result, "UnmergePages", UnmergePagesWrapper);
		NSV8Objects::Template_Set(result, "RedactPage", RedactPageWrapper);
		NSV8Objects::Template_Set(result, "UndoRedact", UndoRedactWrapper);
		NSV8Objects::Template_Set(result, "CheckOwnerPassword", CheckOwnerPasswordWrapper);
		NSV8Objects::Template_Set(result, "CheckPerm", CheckPermWrapper);

		return handle_scope.Escape(result);
	}
}

class CDrawingFileEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSDrawingFileEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CDrawingFileEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CDrawingFileEmbedAdapter();
	return m_pAdapter;
}

std::string CDrawingFileEmbed::getName() { return "CDrawingFileEmbed"; }

CJSEmbedObject* CDrawingFileEmbed::getCreator()
{
	return new CDrawingFileEmbed();
}
