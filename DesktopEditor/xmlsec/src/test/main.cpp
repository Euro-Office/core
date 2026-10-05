#include <cstdio>
#include "../../../common/File.h"
#include "../include/CertificateCommon.h"
#include "../include/OOXMLSigner.h"
#include "../include/OOXMLVerifier.h"

#ifdef _WIN32
//#define USE_MS_CRYPTO
#endif

#define USE_SIGN
#define USE_VERIFY
#define USE_VERIFY_LEGACY

int main()
{
	// Sample files (file.docx, keys/) are expected in the current working
	// directory - CMake copies them next to the built binary in
	// ${EO_CORE_OUTPUT_DIR}, so just run xmlsec_test from there.
	//ICertificate* pCertificate = NSCertificate::FromFiles(L"keys/key.key", "", L"keys/cert.crt", "");

	std::map<std::wstring, std::wstring> properties;
	properties.insert(std::make_pair(L"email", L"sign@onlyoffice.com"));
	properties.insert(std::make_pair(L"phone", L"+00000000000"));
	std::wstring sNameTest = L"NameTest";
	std::wstring sValueTest = L"ValueTest";
	properties.insert(std::make_pair(sNameTest, sValueTest));

	//ICertificate* pCertificate = NSCertificate::GenerateByAlg("rsa2048", properties);
	ICertificate* pCertificate = NSCertificate::GenerateByAlg("ed25519", properties);

	unsigned char* pSignData = NULL;
	unsigned int nSignDataLen = 0;
	std::string sSignData = "Hello world!";
	//bool bRes = pCertificate->SignPKCS7((unsigned char*)sSignData.c_str(), (unsigned int)sSignData.length(), pSignData, nSignDataLen);
	bool bRes = pCertificate->Sign((unsigned char*)sSignData.c_str(), (unsigned int)sSignData.length(), pSignData, nSignDataLen);

	RELEASEARRAYOBJECTS(pSignData);

	BYTE* pDataDst = NULL;
	unsigned long nLenDst = 0;
	int nResult = 0;

#ifdef USE_SIGN
#if 0
	COOXMLSigner oSigner(L"file", pCertificate);
	oSigner.Sign(pDataDst, nLenDst);
#else
	BYTE* pDataSrc = NULL;
	unsigned long nLenSrc = 0;
	NSFile::CFileBinary::ReadAllBytes(L"file.docx", &pDataSrc, nLenSrc);

	COOXMLSigner oSigner(pDataSrc, nLenSrc, pCertificate);
	oSigner.Sign(pDataDst, nLenDst);
	RELEASEARRAYOBJECTS(pDataSrc);

	NSFile::CFileBinary oFileDst;
	oFileDst.CreateFileW(L"file2.docx");
	oFileDst.WriteFile(pDataDst, nLenDst);
	oFileDst.CloseFile();
#endif
#endif

#ifdef USE_VERIFY
#if 1
	// Verify the signed bytes produced by USE_SIGN above directly (the
	// CZipFolderMemory-backed constructor), rather than reading back a
	// "file" path from disk via the other constructor - that one expects an
	// already-extracted OOXML package folder, which nothing here produces,
	// so it always silently found zero signatures.
	COOXMLVerifier oVerifier(pDataDst, nLenDst);
	int nCount = oVerifier.GetSignatureCount();
	printf("Verify: found %d signature(s)\n", nCount);
	if (nCount == 0)
		nResult = 1;

	for (int i = 0; i < nCount; i++)
	{
		COOXMLSignature* pSign = oVerifier.GetSignature(i);
		pSign->Check();
		int nValid = pSign->GetValid();
		printf("Verify: signature %d -> %s (code %d)\n", i,
			(nValid == OOXML_SIGNATURE_VALID) ? "VALID" : "INVALID", nValid);
		if (nValid != OOXML_SIGNATURE_VALID)
			nResult = 1;
	}
#endif
#endif

#ifdef USE_VERIFY_LEGACY
	// Separate from the same-build round trip above: verifies
	// legacy-signed.docx, a document signed by upstream ONLYOFFICE Desktop
	// Editors under an old OpenSSL version, committed as a permanent
	// regression fixture proving documents signed before the 1.1.1w -> 4.0.1
	// migration still verify.
	{
		BYTE* pLegacyData = NULL;
		unsigned long nLegacyLen = 0;
		NSFile::CFileBinary::ReadAllBytes(L"legacy-signed.docx", &pLegacyData, nLegacyLen);

		COOXMLVerifier oLegacyVerifier(pLegacyData, nLegacyLen);
		int nLegacyCount = oLegacyVerifier.GetSignatureCount();
		printf("Legacy verify: found %d signature(s)\n", nLegacyCount);
		if (nLegacyCount == 0)
			nResult = 1;

		for (int i = 0; i < nLegacyCount; i++)
		{
			COOXMLSignature* pSign = oLegacyVerifier.GetSignature(i);
			pSign->Check();
			int nValid = pSign->GetValid();
			printf("Legacy verify: signature %d -> %s (code %d)\n", i,
				(nValid == OOXML_SIGNATURE_VALID) ? "VALID" : "INVALID", nValid);
			if (nValid != OOXML_SIGNATURE_VALID)
				nResult = 1;
		}
		RELEASEARRAYOBJECTS(pLegacyData);
	}
#endif

	RELEASEARRAYOBJECTS(pDataDst);

	delete pCertificate;
	return nResult;
}
