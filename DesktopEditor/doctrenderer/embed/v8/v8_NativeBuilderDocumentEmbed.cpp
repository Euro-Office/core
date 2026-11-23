// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../NativeBuilderDocumentEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSNativeBuilderDocumentEmbed
{
#define CURRENTWRAPPER CBuilderDocumentEmbed

	FUNCTION_WRAPPER_V8_0(IsValidWrapper, IsValid)
	FUNCTION_WRAPPER_V8_0(GetBinaryWrapper, GetBinary)
	FUNCTION_WRAPPER_V8_0(GetFolderWrapper, GetFolder)
	FUNCTION_WRAPPER_V8_0(CloseWrapper, Close)
	FUNCTION_WRAPPER_V8_0(GetImageMapWrapper, GetImageMap)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "IsValid", IsValidWrapper);
		NSV8Objects::Template_Set(result, "GetBinary", GetBinaryWrapper);
		NSV8Objects::Template_Set(result, "GetFolder", GetFolderWrapper);
		NSV8Objects::Template_Set(result, "Close", CloseWrapper);
		NSV8Objects::Template_Set(result, "GetImageMap", GetImageMapWrapper);

		return handle_scope.Escape(result);
	}
}

class CBuilderDocumentEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSNativeBuilderDocumentEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CBuilderDocumentEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CBuilderDocumentEmbedAdapter();
	return m_pAdapter;
}

std::string CBuilderDocumentEmbed::getName() { return "CBuilderDocumentEmbed"; }

CJSEmbedObject* CBuilderDocumentEmbed::getCreator()
{
	return new CBuilderDocumentEmbed();
}
