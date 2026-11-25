// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../GraphicsEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSGraphicsEmbed
{
#define CURRENTWRAPPER CGraphicsEmbed

	FUNCTION_WRAPPER_V8_5(createWrapper, create)
	FUNCTION_WRAPPER_V8_0(DestroyWrapper, Destroy)
	FUNCTION_WRAPPER_V8_0(EndDrawWrapper, EndDraw)
	FUNCTION_WRAPPER_V8_2(put_GlobalAlphaWrapper, put_GlobalAlpha)
	FUNCTION_WRAPPER_V8_0(Start_GlobalAlphaWrapper, Start_GlobalAlpha)
	FUNCTION_WRAPPER_V8_0(End_GlobalAlphaWrapper, End_GlobalAlpha)
	FUNCTION_WRAPPER_V8_4(p_colorWrapper, p_color)
	FUNCTION_WRAPPER_V8_1(p_widthWrapper, p_width)
	FUNCTION_WRAPPER_V8_1(p_dashWrapper, p_dash)
	FUNCTION_WRAPPER_V8_4(b_color1Wrapper, b_color1)
	FUNCTION_WRAPPER_V8_4(b_color2Wrapper, b_color2)
	FUNCTION_WRAPPER_V8_6(transformWrapper, transform)
	FUNCTION_WRAPPER_V8_1(CalculateFullTransformWrapper, CalculateFullTransform)
	FUNCTION_WRAPPER_V8_0(_sWrapper, _s)
	FUNCTION_WRAPPER_V8_0(_eWrapper, _e)
	FUNCTION_WRAPPER_V8_0(_zWrapper, _z)
	FUNCTION_WRAPPER_V8_2(_mWrapper, _m)
	FUNCTION_WRAPPER_V8_2(_lWrapper, _l)
	FUNCTION_WRAPPER_V8_6(_cWrapper, _c)
	FUNCTION_WRAPPER_V8_4(_c2Wrapper, _c2)
	FUNCTION_WRAPPER_V8_0(dsWrapper, ds)
	FUNCTION_WRAPPER_V8_0(dfWrapper, df)
	FUNCTION_WRAPPER_V8_0(saveWrapper, save)
	FUNCTION_WRAPPER_V8_0(restoreWrapper, restore)
	FUNCTION_WRAPPER_V8_0(clipWrapper, clip)
	FUNCTION_WRAPPER_V8_0(resetWrapper, reset)
	FUNCTION_WRAPPER_V8_0(FreeFontWrapper, FreeFont)
	FUNCTION_WRAPPER_V8_0(ClearLastFontWrapper, ClearLastFont)
	FUNCTION_WRAPPER_V8_7(drawImage2Wrapper, drawImage2)
	FUNCTION_WRAPPER_V8_8(drawImageWrapper, drawImage)
	FUNCTION_WRAPPER_V8_0(GetFontWrapper, GetFont)
	FUNCTION_WRAPPER_V8_2(fontWrapper, font)
	FUNCTION_WRAPPER_V8_4(SetFontWrapper, SetFont)
	FUNCTION_WRAPPER_V8_0(GetTextPrWrapper, GetTextPr)
	FUNCTION_WRAPPER_V8_3(FillTextWrapper, FillText)
	FUNCTION_WRAPPER_V8_3(tWrapper, t)
	FUNCTION_WRAPPER_V8_5(FillText2Wrapper, FillText2)
	FUNCTION_WRAPPER_V8_5(t2Wrapper, t2)
	FUNCTION_WRAPPER_V8_3(FillTextCodeWrapper, FillTextCode)
	FUNCTION_WRAPPER_V8_3(tgWrapper, tg)
	FUNCTION_WRAPPER_V8_1(charspaceWrapper, charspace)
	FUNCTION_WRAPPER_V8_2(private_FillGlyphWrapper, private_FillGlyph)
	FUNCTION_WRAPPER_V8_3(private_FillGlyphCWrapper, private_FillGlyphC)
	FUNCTION_WRAPPER_V8_1(private_FillGlyph2Wrapper, private_FillGlyph2)
	FUNCTION_WRAPPER_V8_1(SetIntegerGridWrapper, SetIntegerGrid)
	FUNCTION_WRAPPER_V8_0(GetIntegerGridWrapper, GetIntegerGrid)
	FUNCTION_WRAPPER_V8_3(DrawStringASCIIWrapper, DrawStringASCII)
	FUNCTION_WRAPPER_V8_3(DrawStringASCII2Wrapper, DrawStringASCII2)
	FUNCTION_WRAPPER_V8_5(DrawHeaderEditWrapper, DrawHeaderEdit)
	FUNCTION_WRAPPER_V8_5(DrawFooterEditWrapper, DrawFooterEdit)
	FUNCTION_WRAPPER_V8_3(DrawLockParagraphWrapper, DrawLockParagraph)
	FUNCTION_WRAPPER_V8_4(DrawLockObjectRectWrapper, DrawLockObjectRect)
	FUNCTION_WRAPPER_V8_4(DrawEmptyTableLineWrapper, DrawEmptyTableLine)
	FUNCTION_WRAPPER_V8_4(DrawSpellingLineWrapper, DrawSpellingLine)
	FUNCTION_WRAPPER_V8_5(drawHorLineWrapper, drawHorLine)
	FUNCTION_WRAPPER_V8_5(drawHorLine2Wrapper, drawHorLine2)
	FUNCTION_WRAPPER_V8_5(drawVerLineWrapper, drawVerLine)
	FUNCTION_WRAPPER_V8_7(drawHorLineExtWrapper, drawHorLineExt)
	FUNCTION_WRAPPER_V8_4(rectWrapper, rect)
	FUNCTION_WRAPPER_V8_4(TableRectWrapper, TableRect)
	FUNCTION_WRAPPER_V8_4(AddClipRectWrapper, AddClipRect)
	FUNCTION_WRAPPER_V8_0(RemoveClipRectWrapper, RemoveClipRect)
	FUNCTION_WRAPPER_V8_4(SetClipWrapper, SetClip)
	FUNCTION_WRAPPER_V8_0(RemoveClipWrapper, RemoveClip)
	FUNCTION_WRAPPER_V8_4(drawMailMergeFieldWrapper, drawMailMergeField)
	FUNCTION_WRAPPER_V8_4(drawSearchResultWrapper, drawSearchResult)
	FUNCTION_WRAPPER_V8_2(drawFlowAnchorWrapper, drawFlowAnchor)
	FUNCTION_WRAPPER_V8_0(SavePenWrapper, SavePen)
	FUNCTION_WRAPPER_V8_0(RestorePenWrapper, RestorePen)
	FUNCTION_WRAPPER_V8_0(SaveBrushWrapper, SaveBrush)
	FUNCTION_WRAPPER_V8_0(RestoreBrushWrapper, RestoreBrush)
	FUNCTION_WRAPPER_V8_0(SavePenBrushWrapper, SavePenBrush)
	FUNCTION_WRAPPER_V8_0(RestorePenBrushWrapper, RestorePenBrush)
	FUNCTION_WRAPPER_V8_0(SaveGrStateWrapper, SaveGrState)
	FUNCTION_WRAPPER_V8_0(RestoreGrStateWrapper, RestoreGrState)
	FUNCTION_WRAPPER_V8_0(StartClipPathWrapper, StartClipPath)
	FUNCTION_WRAPPER_V8_0(EndClipPathWrapper, EndClipPath)
	FUNCTION_WRAPPER_V8_0(StartCheckTableDrawWrapper, StartCheckTableDraw)
	FUNCTION_WRAPPER_V8_4(SetTextClipRectWrapper, SetTextClipRect)
	FUNCTION_WRAPPER_V8_5(AddSmartRectWrapper, AddSmartRect)
	FUNCTION_WRAPPER_V8_1(CheckUseFonts2Wrapper, CheckUseFonts2)
	FUNCTION_WRAPPER_V8_0(UncheckUseFonts2Wrapper, UncheckUseFonts2)
	FUNCTION_WRAPPER_V8_4(Drawing_StartCheckBoundsWrapper, Drawing_StartCheckBounds)
	FUNCTION_WRAPPER_V8_0(Drawing_EndCheckBoundsWrapper, Drawing_EndCheckBounds)
	FUNCTION_WRAPPER_V8_5(DrawPresentationCommentWrapper, DrawPresentationComment)
	FUNCTION_WRAPPER_V8_3(DrawPolygonWrapper, DrawPolygon)
	FUNCTION_WRAPPER_V8_4(DrawFootnoteRectWrapper, DrawFootnoteRect)
	FUNCTION_WRAPPER_V8_1(toDataURLWrapper, toDataURL)
	FUNCTION_WRAPPER_V8_0(GetPenColorWrapper, GetPenColor)
	FUNCTION_WRAPPER_V8_0(GetBrushColorWrapper, GetBrushColor)
	FUNCTION_WRAPPER_V8_2(put_brushTextureWrapper, put_brushTexture)
	FUNCTION_WRAPPER_V8_1(put_brushTextureModeWrapper, put_brushTextureMode)
	FUNCTION_WRAPPER_V8_1(put_BrushTextureAlphaWrapper, put_BrushTextureAlpha)
	FUNCTION_WRAPPER_V8_8(put_BrushGradientWrapper, put_BrushGradient)
	FUNCTION_WRAPPER_V8_2(TransformPointXWrapper, TransformPointX)
	FUNCTION_WRAPPER_V8_2(TransformPointYWrapper, TransformPointY)
	FUNCTION_WRAPPER_V8_1(put_LineJoinWrapper, put_LineJoin)
	FUNCTION_WRAPPER_V8_0(get_LineJoinWrapper, get_LineJoin)
	FUNCTION_WRAPPER_V8_4(put_TextureBoundsWrapper, put_TextureBounds)
	FUNCTION_WRAPPER_V8_0(GetlineWidthWrapper, GetlineWidth)
	FUNCTION_WRAPPER_V8_1(DrawPathWrapper, DrawPath)
	FUNCTION_WRAPPER_V8_2(CoordTransformOffsetWrapper, CoordTransformOffset)
	FUNCTION_WRAPPER_V8_0(GetTransformWrapper, GetTransform)
	FUNCTION_WRAPPER_V8_1(CreateLayerWrapper, CreateLayer)
	FUNCTION_WRAPPER_V8_0(BlendLayerWrapper, BlendLayer)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "create", createWrapper);
		NSV8Objects::Template_Set(result, "Destroy", DestroyWrapper);
		NSV8Objects::Template_Set(result, "EndDraw", EndDrawWrapper);
		NSV8Objects::Template_Set(result, "put_GlobalAlpha", put_GlobalAlphaWrapper);
		NSV8Objects::Template_Set(result, "Start_GlobalAlpha", Start_GlobalAlphaWrapper);
		NSV8Objects::Template_Set(result, "End_GlobalAlpha", End_GlobalAlphaWrapper);
		NSV8Objects::Template_Set(result, "p_color", p_colorWrapper);
		NSV8Objects::Template_Set(result, "p_width", p_widthWrapper);
		NSV8Objects::Template_Set(result, "p_dash", p_dashWrapper);
		NSV8Objects::Template_Set(result, "b_color1", b_color1Wrapper);
		NSV8Objects::Template_Set(result, "b_color2", b_color2Wrapper);
		NSV8Objects::Template_Set(result, "transform", transformWrapper);
		NSV8Objects::Template_Set(result, "CalculateFullTransform", CalculateFullTransformWrapper);
		NSV8Objects::Template_Set(result, "_s", _sWrapper);
		NSV8Objects::Template_Set(result, "_e", _eWrapper);
		NSV8Objects::Template_Set(result, "_z", _zWrapper);
		NSV8Objects::Template_Set(result, "_m", _mWrapper);
		NSV8Objects::Template_Set(result, "_l", _lWrapper);
		NSV8Objects::Template_Set(result, "_c", _cWrapper);
		NSV8Objects::Template_Set(result, "_c2", _c2Wrapper);
		NSV8Objects::Template_Set(result, "ds", dsWrapper);
		NSV8Objects::Template_Set(result, "df", dfWrapper);
		NSV8Objects::Template_Set(result, "save", saveWrapper);
		NSV8Objects::Template_Set(result, "restore", restoreWrapper);
		NSV8Objects::Template_Set(result, "clip", clipWrapper);
		NSV8Objects::Template_Set(result, "reset", resetWrapper);
		NSV8Objects::Template_Set(result, "FreeFont", FreeFontWrapper);
		NSV8Objects::Template_Set(result, "ClearLastFont", ClearLastFontWrapper);
		NSV8Objects::Template_Set(result, "drawImage2", drawImage2Wrapper);
		NSV8Objects::Template_Set(result, "drawImage", drawImageWrapper);
		NSV8Objects::Template_Set(result, "GetFont", GetFontWrapper);
		NSV8Objects::Template_Set(result, "font", fontWrapper);
		NSV8Objects::Template_Set(result, "SetFont", SetFontWrapper);
		NSV8Objects::Template_Set(result, "GetTextPr", GetTextPrWrapper);
		NSV8Objects::Template_Set(result, "FillText", FillTextWrapper);
		NSV8Objects::Template_Set(result, "t", tWrapper);
		NSV8Objects::Template_Set(result, "FillText2", FillText2Wrapper);
		NSV8Objects::Template_Set(result, "t2", t2Wrapper);
		NSV8Objects::Template_Set(result, "FillTextCode", FillTextCodeWrapper);
		NSV8Objects::Template_Set(result, "tg", tgWrapper);
		NSV8Objects::Template_Set(result, "charspace", charspaceWrapper);
		NSV8Objects::Template_Set(result, "private_FillGlyph", private_FillGlyphWrapper);
		NSV8Objects::Template_Set(result, "private_FillGlyphC", private_FillGlyphCWrapper);
		NSV8Objects::Template_Set(result, "private_FillGlyph2", private_FillGlyph2Wrapper);
		NSV8Objects::Template_Set(result, "SetIntegerGrid", SetIntegerGridWrapper);
		NSV8Objects::Template_Set(result, "GetIntegerGrid", GetIntegerGridWrapper);
		NSV8Objects::Template_Set(result, "DrawStringASCII", DrawStringASCIIWrapper);
		NSV8Objects::Template_Set(result, "DrawStringASCII2", DrawStringASCII2Wrapper);
		NSV8Objects::Template_Set(result, "DrawHeaderEdit", DrawHeaderEditWrapper);
		NSV8Objects::Template_Set(result, "DrawFooterEdit", DrawFooterEditWrapper);
		NSV8Objects::Template_Set(result, "DrawLockParagraph", DrawLockParagraphWrapper);
		NSV8Objects::Template_Set(result, "DrawLockObjectRect", DrawLockObjectRectWrapper);
		NSV8Objects::Template_Set(result, "DrawEmptyTableLine", DrawEmptyTableLineWrapper);
		NSV8Objects::Template_Set(result, "DrawSpellingLine", DrawSpellingLineWrapper);
		NSV8Objects::Template_Set(result, "drawHorLine", drawHorLineWrapper);
		NSV8Objects::Template_Set(result, "drawHorLine2", drawHorLine2Wrapper);
		NSV8Objects::Template_Set(result, "drawVerLine", drawVerLineWrapper);
		NSV8Objects::Template_Set(result, "drawHorLineExt", drawHorLineExtWrapper);
		NSV8Objects::Template_Set(result, "rect", rectWrapper);
		NSV8Objects::Template_Set(result, "TableRect", TableRectWrapper);
		NSV8Objects::Template_Set(result, "AddClipRect", AddClipRectWrapper);
		NSV8Objects::Template_Set(result, "RemoveClipRect", RemoveClipRectWrapper);
		NSV8Objects::Template_Set(result, "SetClip", SetClipWrapper);
		NSV8Objects::Template_Set(result, "RemoveClip", RemoveClipWrapper);
		NSV8Objects::Template_Set(result, "drawMailMergeField", drawMailMergeFieldWrapper);
		NSV8Objects::Template_Set(result, "drawSearchResult", drawSearchResultWrapper);
		NSV8Objects::Template_Set(result, "drawFlowAnchor", drawFlowAnchorWrapper);
		NSV8Objects::Template_Set(result, "SavePen", SavePenWrapper);
		NSV8Objects::Template_Set(result, "RestorePen", RestorePenWrapper);
		NSV8Objects::Template_Set(result, "SaveBrush", SaveBrushWrapper);
		NSV8Objects::Template_Set(result, "RestoreBrush", RestoreBrushWrapper);
		NSV8Objects::Template_Set(result, "SavePenBrush", SavePenBrushWrapper);
		NSV8Objects::Template_Set(result, "RestorePenBrush", RestorePenBrushWrapper);
		NSV8Objects::Template_Set(result, "SaveGrState", SaveGrStateWrapper);
		NSV8Objects::Template_Set(result, "RestoreGrState", RestoreGrStateWrapper);
		NSV8Objects::Template_Set(result, "StartClipPath", StartClipPathWrapper);
		NSV8Objects::Template_Set(result, "EndClipPath", EndClipPathWrapper);
		NSV8Objects::Template_Set(result, "StartCheckTableDraw", StartCheckTableDrawWrapper);
		NSV8Objects::Template_Set(result, "SetTextClipRect", SetTextClipRectWrapper);
		NSV8Objects::Template_Set(result, "AddSmartRect", AddSmartRectWrapper);
		NSV8Objects::Template_Set(result, "CheckUseFonts2", CheckUseFonts2Wrapper);
		NSV8Objects::Template_Set(result, "UncheckUseFonts2", UncheckUseFonts2Wrapper);
		NSV8Objects::Template_Set(result, "Drawing_StartCheckBounds", Drawing_StartCheckBoundsWrapper);
		NSV8Objects::Template_Set(result, "Drawing_EndCheckBounds", Drawing_EndCheckBoundsWrapper);
		NSV8Objects::Template_Set(result, "DrawPresentationComment", DrawPresentationCommentWrapper);
		NSV8Objects::Template_Set(result, "DrawPolygon", DrawPolygonWrapper);
		NSV8Objects::Template_Set(result, "DrawFootnoteRect", DrawFootnoteRectWrapper);
		NSV8Objects::Template_Set(result, "toDataURL", toDataURLWrapper);
		NSV8Objects::Template_Set(result, "GetPenColor", GetPenColorWrapper);
		NSV8Objects::Template_Set(result, "GetBrushColor", GetBrushColorWrapper);
		NSV8Objects::Template_Set(result, "put_brushTexture", put_brushTextureWrapper);
		NSV8Objects::Template_Set(result, "put_brushTextureMode", put_brushTextureModeWrapper);
		NSV8Objects::Template_Set(result, "put_BrushTextureAlpha", put_BrushTextureAlphaWrapper);
		NSV8Objects::Template_Set(result, "put_BrushGradient", put_BrushGradientWrapper);
		NSV8Objects::Template_Set(result, "TransformPointX", TransformPointXWrapper);
		NSV8Objects::Template_Set(result, "TransformPointY", TransformPointYWrapper);
		NSV8Objects::Template_Set(result, "put_LineJoin", put_LineJoinWrapper);
		NSV8Objects::Template_Set(result, "get_LineJoin", get_LineJoinWrapper);
		NSV8Objects::Template_Set(result, "put_TextureBounds", put_TextureBoundsWrapper);
		NSV8Objects::Template_Set(result, "GetlineWidth", GetlineWidthWrapper);
		NSV8Objects::Template_Set(result, "DrawPath", DrawPathWrapper);
		NSV8Objects::Template_Set(result, "CoordTransformOffset", CoordTransformOffsetWrapper);
		NSV8Objects::Template_Set(result, "GetTransform", GetTransformWrapper);
		NSV8Objects::Template_Set(result, "CreateLayer", CreateLayerWrapper);
		NSV8Objects::Template_Set(result, "BlendLayer", BlendLayerWrapper);

		return handle_scope.Escape(result);
	}
}

class CGraphicsEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSGraphicsEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CGraphicsEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CGraphicsEmbedAdapter();
	return m_pAdapter;
}

std::string CGraphicsEmbed::getName() { return "CGraphicsEmbed"; }

CJSEmbedObject* CGraphicsEmbed::getCreator()
{
	return new CGraphicsEmbed();
}
