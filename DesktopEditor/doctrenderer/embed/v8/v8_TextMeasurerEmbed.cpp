// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../TextMeasurerEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSTextMeasurerEmbed
{
#define CURRENTWRAPPER CTextMeasurerEmbed

#ifdef SUPPORT_HARFBUZZ_SHAPER
	FUNCTION_WRAPPER_V8_1(HB_LanguageFromStringWrapper, HB_LanguageFromString)
	FUNCTION_WRAPPER_V8_7(HB_ShapeTextWrapper, HB_ShapeText)
	FUNCTION_WRAPPER_V8_0(HB_FontMallocWrapper, HB_FontMalloc)
	FUNCTION_WRAPPER_V8_1(HB_FontFreeWrapper, HB_FontFree)
#endif
	FUNCTION_WRAPPER_V8_1(FT_MallocWrapper, FT_Malloc)
	FUNCTION_WRAPPER_V8_1(FT_FreeWrapper, FT_Free)
	FUNCTION_WRAPPER_V8_0(FT_InitWrapper, FT_Init)
	FUNCTION_WRAPPER_V8_2(FT_Set_TrueType_HintPropWrapper, FT_Set_TrueType_HintProp)
	FUNCTION_WRAPPER_V8_4(FT_Open_FaceWrapper, FT_Open_Face)
	FUNCTION_WRAPPER_V8_3(FT_Open_Face2Wrapper, FT_Open_Face2)
	FUNCTION_WRAPPER_V8_1(FT_GetFaceInfoWrapper, FT_GetFaceInfo)
	FUNCTION_WRAPPER_V8_3(FT_Load_GlyphWrapper, FT_Load_Glyph)
	FUNCTION_WRAPPER_V8_2(FT_Get_Glyph_Measure_ParamsWrapper, FT_Get_Glyph_Measure_Params)
	FUNCTION_WRAPPER_V8_2(FT_Get_Glyph_Render_ParamsWrapper, FT_Get_Glyph_Render_Params)
	FUNCTION_WRAPPER_V8_2(FT_Get_Glyph_Render_BufferWrapper, FT_Get_Glyph_Render_Buffer)
	FUNCTION_WRAPPER_V8_5(FT_Set_TransformWrapper, FT_Set_Transform)
	FUNCTION_WRAPPER_V8_5(FT_Set_Char_SizeWrapper, FT_Set_Char_Size)
	FUNCTION_WRAPPER_V8_2(FT_SetCMapForCharCodeWrapper, FT_SetCMapForCharCode)
	FUNCTION_WRAPPER_V8_3(FT_GetKerningXWrapper, FT_GetKerningX)
	FUNCTION_WRAPPER_V8_1(FT_GetFaceMaxAdvanceXWrapper, FT_GetFaceMaxAdvanceX)
	FUNCTION_WRAPPER_V8_1(Hyphen_SetCacheSizeWrapper, Hyphen_SetCacheSize)
	FUNCTION_WRAPPER_V8_2(Hyphen_WordWrapper, Hyphen_Word)
	FUNCTION_WRAPPER_V8_1(Hyphen_IsDictionaryExistWrapper, Hyphen_IsDictionaryExist)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

#ifdef SUPPORT_HARFBUZZ_SHAPER
		NSV8Objects::Template_Set(result, "HB_LanguageFromString", HB_LanguageFromStringWrapper);
		NSV8Objects::Template_Set(result, "HB_ShapeText", HB_ShapeTextWrapper);
		NSV8Objects::Template_Set(result, "HB_FontMalloc", HB_FontMallocWrapper);
		NSV8Objects::Template_Set(result, "HB_FontFree", HB_FontFreeWrapper);
#endif
		NSV8Objects::Template_Set(result, "FT_Malloc", FT_MallocWrapper);
		NSV8Objects::Template_Set(result, "FT_Free", FT_FreeWrapper);
		NSV8Objects::Template_Set(result, "FT_Init", FT_InitWrapper);
		NSV8Objects::Template_Set(result, "FT_Set_TrueType_HintProp", FT_Set_TrueType_HintPropWrapper);
		NSV8Objects::Template_Set(result, "FT_Open_Face", FT_Open_FaceWrapper);
		NSV8Objects::Template_Set(result, "FT_Open_Face2", FT_Open_Face2Wrapper);
		NSV8Objects::Template_Set(result, "FT_GetFaceInfo", FT_GetFaceInfoWrapper);
		NSV8Objects::Template_Set(result, "FT_Load_Glyph", FT_Load_GlyphWrapper);
		NSV8Objects::Template_Set(result, "FT_Get_Glyph_Measure_Params", FT_Get_Glyph_Measure_ParamsWrapper);
		NSV8Objects::Template_Set(result, "FT_Get_Glyph_Render_Params", FT_Get_Glyph_Render_ParamsWrapper);
		NSV8Objects::Template_Set(result, "FT_Get_Glyph_Render_Buffer", FT_Get_Glyph_Render_BufferWrapper);
		NSV8Objects::Template_Set(result, "FT_Set_Transform", FT_Set_TransformWrapper);
		NSV8Objects::Template_Set(result, "FT_Set_Char_Size", FT_Set_Char_SizeWrapper);
		NSV8Objects::Template_Set(result, "FT_SetCMapForCharCode", FT_SetCMapForCharCodeWrapper);
		NSV8Objects::Template_Set(result, "FT_GetKerningX", FT_GetKerningXWrapper);
		NSV8Objects::Template_Set(result, "FT_GetFaceMaxAdvanceX", FT_GetFaceMaxAdvanceXWrapper);
		NSV8Objects::Template_Set(result, "Hyphen_SetCacheSize", Hyphen_SetCacheSizeWrapper);
		NSV8Objects::Template_Set(result, "Hyphen_Word", Hyphen_WordWrapper);
		NSV8Objects::Template_Set(result, "Hyphen_IsDictionaryExist", Hyphen_IsDictionaryExistWrapper);

		return handle_scope.Escape(result);
	}
}

class CTextMeasurerEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSTextMeasurerEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CTextMeasurerEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CTextMeasurerEmbedAdapter();
	return m_pAdapter;
}

std::string CTextMeasurerEmbed::getName() { return "CTextMeasurerEmbed"; }

CJSEmbedObject* CTextMeasurerEmbed::getCreator()
{
	return new CTextMeasurerEmbed();
}
