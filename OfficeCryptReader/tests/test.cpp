/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for OfficeCryptReader's ECMA-376 and ODF encryption, the code that
// uses Crypto++. Decrypting data encrypted by other implementations
// (reference_vectors.h, see generate_vectors.py) checks the algorithms;
// the round trips check the encryptors.

#include "gtest/gtest.h"

#include "../source/CryptTransform.h"

#include "reference_vectors.h"

#include <string>
#include <vector>

using namespace CRYPT;

namespace
{
	const std::wstring c_sPassword = L"Pässwört€";
	const std::wstring c_sAsciiPassword = L"Euro-Office";

	// Same generator as lcg_bytes() in generate_vectors.py
	std::vector<unsigned char> LcgBytes(size_t nCount, unsigned int nSeed)
	{
		std::vector<unsigned char> arData;
		unsigned int x = nSeed;
		for (size_t i = 0; i < nCount; ++i)
		{
			x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu;
			arData.push_back((unsigned char)((x >> 16) & 0xFF));
		}
		return arData;
	}

	template<size_t N>
	std::string Str(const unsigned char (&arData)[N])
	{
		return std::string((const char*)arData, N);
	}

	// As ECMACryptFile reads them from msoffcrypto-tool's EncryptionInfo
	_ecmaCryptData AgileReferenceData()
	{
		_ecmaCryptData oData;
		oData.bAgile = true;
		oData.cipherAlgorithm = oData.dataCipherAlgorithm = CRYPT_METHOD::AES_CBC;
		oData.hashAlgorithm = oData.dataHashAlgorithm = CRYPT_METHOD::SHA512;
		oData.spinCount = 100000;
		oData.keySize = oData.dataKeySize = 32;
		oData.hashSize = oData.dataHashSize = 64;
		oData.blockSize = oData.dataBlockSize = 16;
		oData.saltSize = oData.dataSaltSize = 16;
		oData.saltValue = Str(c_arAgileSalt);
		oData.dataSaltValue = Str(c_arAgileDataSalt);
		oData.encryptedKeyValue = Str(c_arAgileEncryptedKeyValue);
		oData.encryptedVerifierInput = Str(c_arAgileEncryptedVerifierInput);
		oData.encryptedVerifierValue = Str(c_arAgileEncryptedVerifierValue);
		oData.encryptedHmacKey = Str(c_arAgileEncryptedHmacKey);
		oData.encryptedHmacValue = Str(c_arAgileEncryptedHmacValue);
		return oData;
	}

	// The EncryptedPackage stream: 8 byte size, then the encrypted data
	std::vector<unsigned char> DecryptPackage(ECMADecryptor& oDecryptor, std::vector<unsigned char> arPackage)
	{
		unsigned char* pOut = NULL;
		oDecryptor.Decrypt(arPackage.data() + 8, (int)arPackage.size() - 8, pOut, 0);
		size_t nSize = 0;
		for (int i = 7; i >= 0; --i)
			nSize = (nSize << 8) | arPackage[i];
		std::vector<unsigned char> arOut(pOut, pOut + std::min(nSize, arPackage.size() - 8));
		delete[] pOut;
		return arOut;
	}

	std::vector<unsigned char> EncryptPackage(_ecmaCryptData& oData, const std::vector<unsigned char>& arPlain)
	{
		ECMAEncryptor oEncryptor;
		oEncryptor.SetCryptData(oData);
		oEncryptor.SetPassword(c_sPassword);

		std::vector<unsigned char> arInput(arPlain);
		unsigned char* pOut = NULL;
		const int nSize = oEncryptor.Encrypt(arInput.data(), (int)arInput.size(), pOut);
		std::vector<unsigned char> arPackage(pOut, pOut + nSize);
		delete[] pOut;

		oEncryptor.UpdateDataIntegrity(arPackage.data(), (int)arPackage.size());
		oEncryptor.GetCryptData(oData);
		return arPackage;
	}

	_odfCryptData OdfAesData()
	{
		// ODF 1.2 as LibreOffice writes it
		_odfCryptData oData;
		oData.cipherAlgorithm = CRYPT_METHOD::AES_CBC;
		oData.start_hashAlgorithm = CRYPT_METHOD::SHA256;
		oData.start_hashSize = 32;
		oData.spinCount = 1024;
		oData.keySize = 32;
		oData.checksum_size = 1024;
		oData.checksum_hashAlgorithm = CRYPT_METHOD::SHA256;
		return oData;
	}

	bool OdfDecrypt(const _odfCryptData& oData, const std::wstring& sPassword, std::vector<unsigned char> arEncrypted,
					size_t nPlainSize, std::vector<unsigned char>& arPlain)
	{
		_odfCryptData oCopy = oData;
		ODFDecryptor oDecryptor;
		oDecryptor.SetCryptData(oCopy);

		unsigned char* pOut = NULL;
		int nOutSize = (int)nPlainSize;
		const bool bResult = oDecryptor.Decrypt(sPassword, arEncrypted.data(), (int)arEncrypted.size(), pOut, nOutSize);
		if (pOut)
			arPlain.assign(pOut, pOut + nPlainSize);
		delete[] pOut;
		return bResult;
	}
}

// ECMA-376 agile encryption (.docx, .xlsx, .pptx)

TEST(CEcmaAgileTest, DecryptsMsoffcryptoPackage)
{
	_ecmaCryptData oData = AgileReferenceData();
	ECMADecryptor oDecryptor;
	oDecryptor.SetCryptData(oData);
	ASSERT_TRUE(oDecryptor.SetPassword(c_sPassword));

	std::vector<unsigned char> arPackage(std::begin(c_arAgileEncryptedPackage), std::end(c_arAgileEncryptedPackage));
	EXPECT_EQ(LcgBytes(5000, 1), DecryptPackage(oDecryptor, arPackage));
	EXPECT_TRUE(oDecryptor.CheckDataIntegrity(arPackage.data(), (int)arPackage.size()));
}

TEST(CEcmaAgileTest, RejectsWrongPasswordAndTamperedPackage)
{
	_ecmaCryptData oData = AgileReferenceData();
	ECMADecryptor oDecryptor;
	oDecryptor.SetCryptData(oData);
	EXPECT_FALSE(oDecryptor.SetPassword(L"Passwort"));

	ASSERT_TRUE(oDecryptor.SetPassword(c_sPassword));
	std::vector<unsigned char> arPackage(std::begin(c_arAgileEncryptedPackage), std::end(c_arAgileEncryptedPackage));
	arPackage[4100] ^= 0x01;
	EXPECT_FALSE(oDecryptor.CheckDataIntegrity(arPackage.data(), (int)arPackage.size()));
}

TEST(CEcmaAgileTest, RoundTrip)
{
	_ecmaCryptData oData;
	oData.cipherAlgorithm = oData.dataCipherAlgorithm = CRYPT_METHOD::AES_CBC;
	oData.hashAlgorithm = oData.dataHashAlgorithm = CRYPT_METHOD::SHA512;
	oData.keySize = oData.dataKeySize = 32;
	oData.hashSize = oData.dataHashSize = 64;

	const std::vector<unsigned char> arPlain = LcgBytes(10000, 3);
	std::vector<unsigned char> arPackage = EncryptPackage(oData, arPlain);

	ECMADecryptor oDecryptor;
	oDecryptor.SetCryptData(oData);
	ASSERT_TRUE(oDecryptor.SetPassword(c_sPassword));
	EXPECT_EQ(arPlain, DecryptPackage(oDecryptor, arPackage));
	EXPECT_TRUE(oDecryptor.CheckDataIntegrity(arPackage.data(), (int)arPackage.size()));
}

// ECMA-376 standard encryption (Office 2007), as ECMACryptFile sets it up

TEST(CEcmaStandardTest, RoundTripAesAndRc4)
{
	for (CRYPT_METHOD::_cipherAlgorithm eCipher : {CRYPT_METHOD::AES_ECB, CRYPT_METHOD::RC4})
	{
		_ecmaCryptData oData;
		oData.bAgile = false;
		oData.cipherAlgorithm = oData.dataCipherAlgorithm = eCipher;
		oData.hashAlgorithm = oData.dataHashAlgorithm = CRYPT_METHOD::SHA1;
		oData.spinCount = 50000;

		const std::vector<unsigned char> arPlain = LcgBytes(3000, 4);
		std::vector<unsigned char> arPackage = EncryptPackage(oData, arPlain);

		ECMADecryptor oDecryptor;
		oDecryptor.SetCryptData(oData);
		EXPECT_FALSE(oDecryptor.SetPassword(L"Passwort")) << eCipher;
		ASSERT_TRUE(oDecryptor.SetPassword(c_sPassword)) << eCipher;
		EXPECT_EQ(arPlain, DecryptPackage(oDecryptor, arPackage)) << eCipher;
	}
}

// ODF encryption (.odt, .ods, .odp)

TEST(COdfTest, DecryptsReferenceData)
{
	_odfCryptData oData = OdfAesData();
	oData.saltValue = Str(c_arOdfSalt);
	oData.initializationVector = Str(c_arOdfIv);
	oData.checksum = Str(c_arOdfChecksum);
	std::vector<unsigned char> arEncrypted(std::begin(c_arOdfEncrypted), std::end(c_arOdfEncrypted));

	std::vector<unsigned char> arPlain;
	ASSERT_TRUE(OdfDecrypt(oData, c_sPassword, arEncrypted, 3000, arPlain));
	EXPECT_EQ(LcgBytes(3000, 2), arPlain);

	EXPECT_FALSE(OdfDecrypt(oData, L"Passwort", arEncrypted, 3000, arPlain));
}

TEST(COdfTest, RoundTripAesAndBlowfish)
{
	_odfCryptData oBlowfish;
	oBlowfish.cipherAlgorithm = CRYPT_METHOD::Blowfish_CFB; // ODF 1.0 and 1.1
	oBlowfish.checksum_size = 1024;

	for (_odfCryptData oParams : {OdfAesData(), oBlowfish})
	{
		const std::vector<unsigned char> arPlain = LcgBytes(2000, 5);

		ODFEncryptor oEncryptor;
		oEncryptor.SetCryptData(oParams);
		std::vector<unsigned char> arInput(arPlain);
		unsigned char* pOut = NULL;
		const int nSize = oEncryptor.Encrypt(c_sPassword, arInput.data(), (int)arInput.size(), pOut);
		std::vector<unsigned char> arEncrypted(pOut, pOut + nSize);
		delete[] pOut;
		_odfCryptData oData;
		oEncryptor.GetCryptData(oData);

		std::vector<unsigned char> arDecrypted;
		ASSERT_TRUE(OdfDecrypt(oData, c_sPassword, arEncrypted, arPlain.size(), arDecrypted)) << oParams.cipherAlgorithm;
		EXPECT_EQ(arPlain, arDecrypted) << oParams.cipherAlgorithm;
		EXPECT_FALSE(OdfDecrypt(oData, L"Passwort", arEncrypted, arPlain.size(), arDecrypted)) << oParams.cipherAlgorithm;
	}
}

// Write protection ("open read-only unless the password is given")

TEST(CWriteProtectTest, VerifiesReferenceHashes)
{
	_ecmaWriteProtectData oEcmaData;
	oEcmaData.hashAlgorithm = CRYPT_METHOD::SHA512;
	oEcmaData.spinCount = 100000;
	oEcmaData.saltValue = Str(c_arEcmaProtectSalt);
	oEcmaData.hashValue = Str(c_arEcmaProtectHash);
	ECMAWriteProtect oEcma;
	oEcma.SetCryptData(oEcmaData);
	oEcma.SetPassword(c_sPassword);
	EXPECT_TRUE(oEcma.Verify());
	oEcma.SetPassword(L"Passwort");
	EXPECT_FALSE(oEcma.Verify());

	_odfWriteProtectData oOdfData;
	oOdfData.hashAlgorithm = CRYPT_METHOD::PBKDF2;
	oOdfData.spinCount = 100000;
	oOdfData.saltValue = Str(c_arOdfProtectSalt);
	oOdfData.hashValue = Str(c_arOdfProtectHash);
	ODFWriteProtect oOdf;
	oOdf.SetCryptData(oOdfData);
	oOdf.SetPassword(c_sAsciiPassword);
	EXPECT_TRUE(oOdf.Verify());
	oOdf.SetPassword(L"Passwort");
	EXPECT_FALSE(oOdf.Verify());
}

TEST(CWriteProtectTest, RoundTrip)
{
	ECMAWriteProtect oEcma;
	oEcma.SetPassword(c_sPassword);
	oEcma.Generate();
	EXPECT_TRUE(oEcma.Verify());

	_odfWriteProtectData oOdfData;
	oOdfData.hashAlgorithm = CRYPT_METHOD::PBKDF2;
	ODFWriteProtect oOdf;
	oOdf.SetCryptData(oOdfData);
	oOdf.SetPassword(c_sAsciiPassword);
	oOdf.Generate();
	EXPECT_TRUE(oOdf.Verify());
}
