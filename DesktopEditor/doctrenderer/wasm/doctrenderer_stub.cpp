/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * doctrenderer without a JavaScript engine, for the WebAssembly build of x2t.
 *
 * In the browser the editor itself (sdkjs) is the JavaScript side, so every
 * conversion that would run sdkjs inside x2t (bin -> pdf/html rendering, the
 * JS caches and snapshots, document builder scripts) is reported as failed
 * here and has to be done by the editor instead. The approach follows
 * CryptPad's onlyoffice-x2t-wasm, which ships x2t without V8 the same way.
 */

#include "../doctrenderer.h"
#include "../docbuilder.h"

namespace NSDoctRenderer
{
	class CDoctRenderer_Private
	{
	};

	CDoctrenderer::CDoctrenderer(const std::wstring& sAllFontsPath)
	{
		m_pInternal = NULL;
	}

	CDoctrenderer::~CDoctrenderer()
	{
	}

	void CDoctrenderer::LoadConfig(const std::wstring& sConfigDir, const std::wstring& sAllFontsPath)
	{
	}

	bool CDoctrenderer::Execute(const std::wstring& strXml, std::wstring& strError)
	{
		strError = L"code=\"wasm\" message=\"doctrenderer is not available in the WebAssembly build\"";
		return false;
	}

	std::vector<std::wstring> CDoctrenderer::GetImagesInChanges()
	{
		return std::vector<std::wstring>();
	}

	void CDoctrenderer::CreateCache(const std::wstring& sAllFontsPath, const std::wstring& sCacheDir)
	{
	}

	void CDoctrenderer::CreateSnapshots()
	{
	}

	void CDoctrenderer::SetAdditionalParam(const AdditionalParamType& type, void* data)
	{
	}

	// x2t only initializes and disposes the builder around a conversion
	void CDocBuilder::Initialize(const wchar_t* directory)
	{
	}

	void CDocBuilder::Dispose()
	{
	}

	void CDocBuilder::SetX2tConverter(int (*converter)(const wchar_t* paramsPath))
	{
	}
}
