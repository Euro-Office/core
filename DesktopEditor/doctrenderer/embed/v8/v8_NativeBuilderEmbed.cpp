// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../NativeBuilderEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSNativeBuilderEmbed
{
#define CURRENTWRAPPER CBuilderEmbed

	FUNCTION_WRAPPER_V8_2(OpenFileWrapper, OpenFile)
	FUNCTION_WRAPPER_V8_1(CreateFileWrapper, CreateFile)
	FUNCTION_WRAPPER_V8_1(SetTmpFolderWrapper, SetTmpFolder)
	FUNCTION_WRAPPER_V8_3(SaveFileWrapper, SaveFile)
	FUNCTION_WRAPPER_V8_0(CloseFileWrapper, CloseFile)
	FUNCTION_WRAPPER_V8_2(OpenTmpFileWrapper, OpenTmpFile)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "OpenFile", OpenFileWrapper);
		NSV8Objects::Template_Set(result, "CreateFile", CreateFileWrapper);
		NSV8Objects::Template_Set(result, "SetTmpFolder", SetTmpFolderWrapper);
		NSV8Objects::Template_Set(result, "SaveFile", SaveFileWrapper);
		NSV8Objects::Template_Set(result, "CloseFile", CloseFileWrapper);
		NSV8Objects::Template_Set(result, "OpenTmpFile", OpenTmpFileWrapper);

		return handle_scope.Escape(result);
	}
}

class CBuilderEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSNativeBuilderEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CBuilderEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CBuilderEmbedAdapter();
	return m_pAdapter;
}

std::string CBuilderEmbed::getName() { return "CBuilderEmbed"; }

CJSEmbedObject* CBuilderEmbed::getCreator()
{
	return new CBuilderEmbed();
}
