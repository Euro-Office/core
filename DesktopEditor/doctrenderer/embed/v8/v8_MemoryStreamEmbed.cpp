// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../MemoryStreamEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSMemoryStreamEmbed
{
#define CURRENTWRAPPER CMemoryStreamEmbed

	FUNCTION_WRAPPER_V8_3(CopyWrapper, Copy)
	FUNCTION_WRAPPER_V8_0(ClearNoAttackWrapper, ClearNoAttack)
	FUNCTION_WRAPPER_V8_1(WriteByteWrapper, WriteByte)
	FUNCTION_WRAPPER_V8_1(WriteBoolWrapper, WriteBool)
	FUNCTION_WRAPPER_V8_1(WriteLongWrapper, WriteLong)
	FUNCTION_WRAPPER_V8_1(WriteDoubleWrapper, WriteDouble)
	FUNCTION_WRAPPER_V8_1(WriteDouble2Wrapper, WriteDouble2)
	FUNCTION_WRAPPER_V8_1(WriteStringAWrapper, WriteStringA)
	FUNCTION_WRAPPER_V8_1(WriteStringWrapper, WriteString)
	FUNCTION_WRAPPER_V8_1(WriteString2Wrapper, WriteString2)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "Copy", CopyWrapper);
		NSV8Objects::Template_Set(result, "ClearNoAttack", ClearNoAttackWrapper);
		NSV8Objects::Template_Set(result, "WriteByte", WriteByteWrapper);
		NSV8Objects::Template_Set(result, "WriteBool", WriteBoolWrapper);
		NSV8Objects::Template_Set(result, "WriteLong", WriteLongWrapper);
		NSV8Objects::Template_Set(result, "WriteDouble", WriteDoubleWrapper);
		NSV8Objects::Template_Set(result, "WriteDouble2", WriteDouble2Wrapper);
		NSV8Objects::Template_Set(result, "WriteStringA", WriteStringAWrapper);
		NSV8Objects::Template_Set(result, "WriteString", WriteStringWrapper);
		NSV8Objects::Template_Set(result, "WriteString2", WriteString2Wrapper);

		return handle_scope.Escape(result);
	}
}

class CMemoryStreamEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSMemoryStreamEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CMemoryStreamEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CMemoryStreamEmbedAdapter();
	return m_pAdapter;
}

std::string CMemoryStreamEmbed::getName() { return "CMemoryStreamEmbed"; }

CJSEmbedObject* CMemoryStreamEmbed::getCreator()
{
	return new CMemoryStreamEmbed();
}
