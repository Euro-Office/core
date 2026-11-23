#include "MemoryStreamEmbed.h"

JSSmart<CJSValue> CMemoryStreamEmbed::Copy(JSSmart<CJSValue> stream, JSSmart<CJSValue> pos, JSSmart<CJSValue> len)
{
	JSSmart<CJSObject> _obj = stream->toObject();
	NSMemoryStream::CMemoryStream* pStream = (NSMemoryStream::CMemoryStream*)(_obj->getNative());
	m_pInternal->Copy(pStream, pos->toInt32(), len->toInt32());
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::ClearNoAttack()
{
	m_pInternal->ClearNoAttack();
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteByte(JSSmart<CJSValue> value)
{
	m_pInternal->WriteBYTE((BYTE)value->toInt32());
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteBool(JSSmart<CJSValue> value)
{
	m_pInternal->WriteBYTE(value->toBool() ? 1 : 0);
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteLong(JSSmart<CJSValue> value)
{
	m_pInternal->WriteLONG((LONG)value->toInt32());
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteDouble(JSSmart<CJSValue> value)
{
	m_pInternal->WriteLONG((LONG)(value->toDouble() * 100000));
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteDouble2(JSSmart<CJSValue> value)
{
	m_pInternal->WriteDouble(value->toDouble());
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteStringA(JSSmart<CJSValue> value)
{
	std::string param = value->toStringA();
	m_pInternal->WriteStringA2(param.c_str(), (int)param.length());
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteString(JSSmart<CJSValue> value)
{
	std::wstring param = value->toStringW();
	m_pInternal->WriteString(param.c_str(), (int)param.length());
	return nullptr;
}

JSSmart<CJSValue> CMemoryStreamEmbed::WriteString2(JSSmart<CJSValue> value)
{
	std::wstring param = value->toStringW();
	m_pInternal->WriteString2(param.c_str(), (int)param.length());
	return nullptr;
}
