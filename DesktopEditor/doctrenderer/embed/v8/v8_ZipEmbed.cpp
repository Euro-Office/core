// THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT CHANGE IT!
// IF YOU NEED TO UPDATE THIS CODE, JUST RERUN PYTHON SCRIPT WITH "--internal" OPTION.

#include "../ZipEmbed.h"
#include "../../js_internal/v8/v8_base.h"

namespace NSZipEmbed
{
#define CURRENTWRAPPER CZipEmbed

	FUNCTION_WRAPPER_V8_1(openWrapper, open)
	FUNCTION_WRAPPER_V8_0(createWrapper, create)
	FUNCTION_WRAPPER_V8_0(saveWrapper, save)
	FUNCTION_WRAPPER_V8_1(getFileWrapper, getFile)
	FUNCTION_WRAPPER_V8_2(addFileWrapper, addFile)
	FUNCTION_WRAPPER_V8_1(removeFileWrapper, removeFile)
	FUNCTION_WRAPPER_V8_0(closeWrapper, close)
	FUNCTION_WRAPPER_V8_0(getPathsWrapper, getPaths)
	FUNCTION_WRAPPER_V8_2(decodeImageWrapper, decodeImage)
	FUNCTION_WRAPPER_V8_6(encodeImageDataWrapper, encodeImageData)
	FUNCTION_WRAPPER_V8_2(encodeImageWrapper, encodeImage)
	FUNCTION_WRAPPER_V8_1(getImageTypeWrapper, getImageType)
	FUNCTION_WRAPPER_V8_1(getImageBufferWrapper, getImageBuffer)

	v8::Local<v8::ObjectTemplate> CreateTemplate(v8::Isolate* isolate)
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> result = v8::ObjectTemplate::New(isolate);
		result->SetInternalFieldCount(1);

		NSV8Objects::Template_Set(result, "open", openWrapper);
		NSV8Objects::Template_Set(result, "create", createWrapper);
		NSV8Objects::Template_Set(result, "save", saveWrapper);
		NSV8Objects::Template_Set(result, "getFile", getFileWrapper);
		NSV8Objects::Template_Set(result, "addFile", addFileWrapper);
		NSV8Objects::Template_Set(result, "removeFile", removeFileWrapper);
		NSV8Objects::Template_Set(result, "close", closeWrapper);
		NSV8Objects::Template_Set(result, "getPaths", getPathsWrapper);
		NSV8Objects::Template_Set(result, "decodeImage", decodeImageWrapper);
		NSV8Objects::Template_Set(result, "encodeImageData", encodeImageDataWrapper);
		NSV8Objects::Template_Set(result, "encodeImage", encodeImageWrapper);
		NSV8Objects::Template_Set(result, "getImageType", getImageTypeWrapper);
		NSV8Objects::Template_Set(result, "getImageBuffer", getImageBufferWrapper);

		return handle_scope.Escape(result);
	}
}

class CZipEmbedAdapter : public CJSEmbedObjectAdapterV8Template
{
public:
	virtual v8::Local<v8::ObjectTemplate> getTemplate(v8::Isolate* isolate) override
	{
		v8::EscapableHandleScope handle_scope(isolate);
		v8::Local<v8::ObjectTemplate> templ = NSZipEmbed::CreateTemplate(isolate);
		return handle_scope.Escape(templ);
	}
};

CJSEmbedObjectAdapterBase* CZipEmbed::getAdapter()
{
	if (m_pAdapter == nullptr)
		m_pAdapter = new CZipEmbedAdapter();
	return m_pAdapter;
}

std::string CZipEmbed::getName() { return "CZipEmbed"; }

CJSEmbedObject* CZipEmbed::getCreator()
{
	return new CZipEmbed();
}
