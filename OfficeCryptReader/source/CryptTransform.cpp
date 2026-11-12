/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/format.hpp>

#include "CryptTransform.h"

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/des.h>
#include <cryptopp/sha.h>
#include <cryptopp/md5.h>
#include <cryptopp/rsa.h>
#include <cryptopp/rc2.h>
#include <cryptopp/arc4.h>
#include <cryptopp/rc5.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/zinflate.h>
#include <cryptopp/zdeflate.h>

#include "../../OOXML/Base/unicode_util.h"
#include "../../OOXML/Base/Base.h"

#include "../../DesktopEditor/common/File.h"
static const unsigned char encrVerifierHashInputBlockKey[8]			= { 0xfe, 0xa7, 0xd2, 0x76, 0x3b, 0x4b, 0x9e, 0x79 };
static const unsigned char encrVerifierHashValueBlockKey[8]			= { 0xd7, 0xaa, 0x0f, 0x6d, 0x30, 0x61, 0x34, 0x4e };
static const unsigned char encrKeyValueBlockKey[8]					= { 0x14, 0x6e, 0x0b, 0xe7, 0xab, 0xac, 0xd0, 0xd6 };

static const unsigned char encrDataIntegritySaltBlockKey[8]			= { 0x5f, 0xb2, 0xad, 0x01, 0x0c, 0xb9, 0xe1, 0xf6 };
static const unsigned char encrDataIntegrityHmacValueBlockKey[8]	= { 0xa0, 0x67, 0x7f, 0x02, 0xb2, 0x2c, 0x84, 0x33 };

#define PADDING_SIZE 16

using namespace CryptoPP;

class buf
{
public:
	unsigned char	*ptr;
	int				size;
//-----------------------------------------------------------------------------------
	buf()													: ptr(nullptr), size(0), 	bDelete(true){}
	buf(int sz, bool zero_set = false)						: ptr(nullptr), size(0), 	bDelete(true)
	{
		ptr = new unsigned char [sz]; 
		size = sz; 
		
		if (ptr && zero_set)
		{
			memset(ptr, 0, size);
		}
	}
	buf(unsigned char * buf, int sz, bool bDelete_ = true ): ptr(nullptr), size(0), 	bDelete(true)
	{
		CreateCopy(buf, sz, bDelete_);
	}
	buf(const std::string  & str)							: ptr(nullptr), size(0), 	bDelete(true)
	{
		CreateCopy((unsigned char*)str.c_str(), str.length(), true);
	}
	buf(const std::wstring & str)							: ptr(nullptr), size(0), 	bDelete(true)
	{
		if (sizeof(wchar_t) == 4)
		{
			unsigned int nLength = str.length();
			
			size = nLength * 2; 
			ptr = new unsigned char [size]; 

			UTF16* pStrUtf16 = (UTF16*)		ptr;
			UTF32 *pStrUtf32 = (UTF32 *)	str.c_str();

            UTF16 *pStrUtf16_Conv		= pStrUtf16;
            const UTF32 *pStrUtf32_Conv = pStrUtf32;

            ConversionResult eUnicodeConversionResult = ConvertUTF32toUTF16 (&pStrUtf32_Conv, &pStrUtf32[nLength]
                    , &pStrUtf16_Conv, &pStrUtf16 [nLength], strictConversion);

            if (conversionOK != eUnicodeConversionResult)
            {
            }
		}
		else
		{
			CreateCopy((unsigned char*)str.c_str(), str.length() * 2, true);
		}
	}
	
	virtual ~buf()	
	{
		Clear();
	}
	buf& operator=(const buf& oSrc)
	{
		if (this == &oSrc) return *this;

		Clear();

		size	= oSrc.size;
		ptr		= new unsigned char [oSrc.size];
		if (oSrc.ptr)
		{
			memcpy(ptr, oSrc.ptr, oSrc.size);
		}
		else
		{
			memset(ptr, 0, oSrc.size);
		}
		bDelete = true;

		return *this;
	}

	void Clear()
	{
		if (bDelete && ptr) delete []ptr; 
		ptr = nullptr; size = 0;

		bDelete = true;
	}
//----------------------------------------------------------------------
private:
	bool bDelete;
	void CreateCopy(unsigned char* buf, int sz, bool bDelete_)
	{
		Clear();
		bDelete = bDelete_;
		if (bDelete)
		{
			ptr = new unsigned char [sz]; size = sz;
			memcpy(ptr, buf , sz);
		}
		else
		{
			ptr = buf; size = sz; 
		}

	}
};

bool operator==(const buf& oBuf1, const buf& oBuf2)
{
	if (!oBuf1.ptr || !oBuf2.ptr) return false;

	return 0 == memcmp(oBuf1.ptr, oBuf2.ptr, (std::min)(oBuf1.size, oBuf2.size));
}

void CorrectHashSize(buf & hashBuf, int size, unsigned char padding)
{
	if (hashBuf.size < size)
	{
		unsigned char *newPtr = new unsigned char[size];
		memset(newPtr, padding, size);
		memcpy(newPtr, hashBuf.ptr, hashBuf.size);
		
		hashBuf.Clear();
		
		hashBuf.ptr		= newPtr;
		hashBuf.size	= size;
	}
	else if (hashBuf.size > size)
	{
		hashBuf.size = size;
	}
}
buf Hmac(buf &  keyBuf, CRYPT_METHOD::_hashAlgorithm algorithm, std::string & plain)
{
	std::string mac;
	if (algorithm == CRYPT_METHOD::SHA1)
	{
        HMAC<SHA1> hmac(keyBuf.ptr, keyBuf.size);
        StringSource(plain, true,
                    new HashFilter(hmac,
                        new StringSink(mac)
					) // HashFilter      
                ); // StringSource

	}
	else if (algorithm == CRYPT_METHOD::SHA256)
	{
        HMAC<SHA256> hmac(keyBuf.ptr, keyBuf.size);
        StringSource(plain, true,
                    new HashFilter(hmac,
                        new StringSink(mac)
					) // HashFilter      
				); // StringSource

	}
	else if (algorithm == CRYPT_METHOD::SHA512)
	{
        HMAC<SHA512> hmac(keyBuf.ptr, keyBuf.size);
		
        StringSource(plain, true,
                    new HashFilter(hmac,
                        new StringSink(mac)
					) // HashFilter      
				); // StringSource

	}
	
	return buf(mac);
}


buf HashAppend(buf &  hashBuf, buf & block, CRYPT_METHOD::_hashAlgorithm algorithm)
{//TODO redo
	if (algorithm == CRYPT_METHOD::MD5)
	{		
		MD5 hash;

		if (hashBuf.ptr && hashBuf.size > 0)	hash.Update( hashBuf.ptr, hashBuf.size);
		if (block.ptr	&& block.size > 0)		hash.Update( block.ptr	, block.size);

		SecByteBlock buffer(hash.DigestSize());
		hash.Final(buffer);

		return buf(buffer.BytePtr(), buffer.SizeInBytes());	
	}
	else if (algorithm == CRYPT_METHOD::SHA1)
	{
		SHA1 hash;

		if (hashBuf.ptr && hashBuf.size > 0)	hash.Update( hashBuf.ptr, hashBuf.size);
		if (block.ptr	&& block.size > 0)		hash.Update( block.ptr	, block.size);

		SecByteBlock buffer(hash.DigestSize());
		hash.Final(buffer);

		return buf(buffer.BytePtr(), buffer.SizeInBytes());
	}
	else if (algorithm == CRYPT_METHOD::SHA256)
	{
		SHA256 hash;

		if (hashBuf.ptr && hashBuf.size > 0)	hash.Update( hashBuf.ptr, hashBuf.size);
		if (block.ptr	&& block.size > 0)		hash.Update( block.ptr	, block.size);

		SecByteBlock buffer(hash.DigestSize());
		hash.Final(buffer);

		return buf(buffer.BytePtr(), buffer.SizeInBytes());
	}
	else if (algorithm == CRYPT_METHOD::SHA512)
	{
		SHA512 hash;

		if (hashBuf.ptr && hashBuf.size > 0)	hash.Update( hashBuf.ptr, hashBuf.size);
		if (block.ptr	&& block.size > 0)		hash.Update( block.ptr	, block.size);

		SecByteBlock buffer(hash.DigestSize());
		hash.Final(buffer);

		return buf(buffer.BytePtr(), buffer.SizeInBytes());
	}
	else
		return buf();
}

buf GenerateAgileKey(buf & salt, buf & password, buf & blockKey, int hashSize, int spin, CRYPT_METHOD::_hashAlgorithm algorithm)
{
	//if (hashSize < 8)
	//	blockKey = 8;

	buf pHashBuf = HashAppend(salt, password, algorithm);

	for (int i = 0; i < spin; i++)
	{
        buf iterator((unsigned char*)&i, 4, false);
        pHashBuf = HashAppend(iterator, pHashBuf, algorithm);
	}

	pHashBuf = HashAppend(pHashBuf, blockKey, algorithm);

	CorrectHashSize(pHashBuf, hashSize, 0x36);

	return buf(pHashBuf.ptr, pHashBuf.size);
}
buf GenerateOdfKey(buf & pSalt, buf & pPassword, int keySize, int spin, CRYPT_METHOD::_hashAlgorithm algorithm)
{
	buf pKey		(keySize);
	buf empty		(nullptr, 0, false);
    buf pPassword_hash;

    if (algorithm == CRYPT_METHOD::None)
    {
        pPassword_hash = pPassword;
    }
    else
    {
        pPassword_hash = HashAppend(pPassword, empty, algorithm);
    }

	PKCS5_PBKDF2_HMAC<SHA1> pbkdf;
		
	pbkdf.DeriveKey(pKey.ptr, pKey.size, 0, pPassword_hash.ptr, pPassword_hash.size, pSalt.ptr, pSalt.size, spin);

	return buf(pKey.ptr, pKey.size);
}
    
buf GenerateHashKey(buf & salt, buf & password, int hashSize, int keySize, int spin, CRYPT_METHOD::_hashAlgorithm algorithm, int block_index = 0)
{
	buf block	((unsigned char*)&block_index, 4, false);

	buf empty	(nullptr, 0, false);
	buf pHashBuf = HashAppend(salt, password, algorithm);
		
	for (int i = 0; i < spin; i++)
	{
        buf iterator((unsigned char*)&i, 4, false);
        pHashBuf = HashAppend(iterator, pHashBuf, algorithm);
	}

    pHashBuf = HashAppend( pHashBuf, block, algorithm);
	
	if (spin == 0)
	{
		return buf(pHashBuf.ptr, pHashBuf.size);
	}

	buf derivedKey1(64);
	memset (derivedKey1.ptr, 0x36, 64);
	for (int i = 0; i < pHashBuf.size; i++)
	{
		derivedKey1.ptr[i] ^= pHashBuf.ptr[i];
	}

	buf derivedKey2(64);
	memset (derivedKey2.ptr, 0x5c, 64);
	for (int i = 0; i < pHashBuf.size; i++)
	{
		derivedKey2.ptr[i] ^= pHashBuf.ptr[i];
	}

	buf pHashBuf1 = HashAppend(derivedKey1, empty, algorithm);
	buf pHashBuf2 = HashAppend(derivedKey2, empty, algorithm);

	buf pHashBuf3(2 * hashSize);
	
	memcpy(pHashBuf3.ptr + 0,			pHashBuf1.ptr, hashSize);
	memcpy(pHashBuf3.ptr + hashSize,	pHashBuf2.ptr, hashSize);

	CorrectHashSize(pHashBuf3, keySize, 0);
	
	if (keySize == 5)
		CorrectHashSize(pHashBuf3, 16, 0); //40-bit crypt key !!!				

	return buf(pHashBuf3.ptr, pHashBuf3.size);
}
ARC4::Decryption rc4Decryption; // TODO -> in impl
ARC4::Encryption rc4Encryption;

bool EncryptCipher(buf & key, buf & iv, buf & data_inp, buf & data_out, CRYPT_METHOD::_cipherAlgorithm algorithm,
				    StreamTransformationFilter::BlockPaddingScheme padding = StreamTransformationFilter::PKCS_PADDING)
{	
	if (algorithm == CRYPT_METHOD::XOR)
	{
		return false;
	}
	else if (algorithm == CRYPT_METHOD::RC4)
	{
		if (!data_out.ptr)
		{
			data_out = buf(data_inp.size);
		}
		
		rc4Encryption.ProcessData(data_out.ptr, data_inp.ptr, data_inp.size);

	}
	else if (algorithm == CRYPT_METHOD::Blowfish_CFB)
	{
		CFB_Mode<Blowfish>::Encryption encryption(key.ptr, key.size, iv.ptr);
		encryption.ProcessData(data_out.ptr, data_inp.ptr, data_inp.size);
	}
	else if (algorithm == CRYPT_METHOD::DES_CBC)
	{
		if (!iv.ptr)
		{
			iv = buf(16, true);
		}
		DES::Encryption desEncryption(key.ptr, key.size == 7 ? 8 : key.size);
		StreamTransformation *modeEncryption = new CipherModeFinalTemplate_ExternalCipher<CBC_Encryption>(desEncryption, iv.ptr ); 
		if (!data_out.ptr)
		{
			data_out = buf(data_inp.size);
		}
		StreamTransformationFilter stfEncryption(*modeEncryption, new ArraySink( data_out.ptr, data_out.size), padding);
	 
		stfEncryption.Put( data_inp.ptr, data_inp.size );
		stfEncryption.MessageEnd();

		delete modeEncryption;
	}
	else if (algorithm == CRYPT_METHOD::DES_ECB)
	{
		if (!iv.ptr)
		{
			iv = buf(16, true);
		}
		DES::Encryption desEncryption(key.ptr, key.size == 7 ? 8 : key.size);
		StreamTransformation *modeEncryption = new CipherModeFinalTemplate_ExternalCipher<ECB_OneWay>(desEncryption, iv.ptr ); 
		if (!data_out.ptr)
		{
			data_out = buf(data_inp.size);
		}
		StreamTransformationFilter stfEncryption(*modeEncryption, new ArraySink( data_out.ptr, data_out.size), padding);
	 
		stfEncryption.Put( data_inp.ptr, data_inp.size );
		stfEncryption.MessageEnd();

		delete modeEncryption;
	}
	else //AES
	{
		StreamTransformation *modeEncryption = nullptr;
		AES::Encryption aesEncryption(key.ptr, key.size);

		switch(algorithm)
		{
		case CRYPT_METHOD::AES_ECB:
			 modeEncryption = new CipherModeFinalTemplate_ExternalCipher<ECB_OneWay>(aesEncryption, iv.ptr ); 
			 break;
		case CRYPT_METHOD::AES_CBC:
			 modeEncryption = new CipherModeFinalTemplate_ExternalCipher<CBC_Encryption>(aesEncryption, iv.ptr ); 
			 break;
//		case CRYPT_METHOD::AES_CFB:
//#if defined(_IOS)
//            return false;
//#else
//			modeEncryption = new CFB_Mode_ExternalCipher::Encryption(aesEncryption, iv.ptr );
//#endif
			break;
		}
	
		if (!modeEncryption) return false;
		    
		if (!data_out.ptr)
		{
			data_out = buf(data_inp.size);
		}
		StreamTransformationFilter stfEncryption(*modeEncryption, new ArraySink( data_out.ptr, data_out.size), padding);
	 
		stfEncryption.Put( data_inp.ptr, data_inp.size );
		stfEncryption.MessageEnd();

		delete modeEncryption;
	}

	return true;
}

bool DecryptCipher(buf & key, buf & iv, buf & data_inp, buf & data_out,  CRYPT_METHOD::_cipherAlgorithm algorithm, 
				  StreamTransformationFilter::BlockPaddingScheme padding = StreamTransformationFilter::NO_PADDING)
{	
	if (!data_out.ptr)
	{
		data_out = buf(data_inp.size);
	}

	if (algorithm == CRYPT_METHOD::XOR)
	{
		return false;
	}
	else if (algorithm == CRYPT_METHOD::RC4)
	{
		rc4Decryption.ProcessData(data_out.ptr, data_inp.ptr, data_inp.size);
	}
	else if (algorithm == CRYPT_METHOD::Blowfish_CFB)
	{
		CFB_Mode<Blowfish>::Decryption decryption(key.ptr, key.size, iv.ptr);
		decryption.ProcessData(data_out.ptr, data_inp.ptr, data_inp.size);
	}
	else if (algorithm == CRYPT_METHOD::DES_CBC)
	{
		if (!iv.ptr)
		{
			iv = buf(16, true);
		}
		DES::Decryption desDecryption(key.ptr, key.size == 7 ? 8 : key.size);
		StreamTransformation *modeDecryption = new CipherModeFinalTemplate_ExternalCipher<CBC_Decryption>(desDecryption, iv.ptr );
		StreamTransformationFilter stfDecryptor(*modeDecryption, new ArraySink( data_out.ptr, data_out.size), padding);
	 
		stfDecryptor.Put( data_inp.ptr, data_inp.size );
		stfDecryptor.MessageEnd();

		delete modeDecryption;
	}
	else if (algorithm == CRYPT_METHOD::DES_ECB)
	{
		if (!iv.ptr)
		{
			iv = buf(16, true);
		}
		DES::Decryption desDecryption(key.ptr, key.size == 7 ? 8 : key.size);
		StreamTransformation *modeDecryption = new CipherModeFinalTemplate_ExternalCipher<ECB_OneWay>(desDecryption, iv.ptr );
		StreamTransformationFilter stfDecryptor(*modeDecryption, new ArraySink( data_out.ptr, data_out.size), padding);
	 
		stfDecryptor.Put( data_inp.ptr, data_inp.size );
		stfDecryptor.MessageEnd();

		delete modeDecryption;
	}
	else //AES
	{
		StreamTransformation *modeDecryption = nullptr;
		AES::Decryption aesDecryption(key.ptr, key.size);

		switch(algorithm)
		{
		case CRYPT_METHOD::AES_ECB:
			 modeDecryption = new CipherModeFinalTemplate_ExternalCipher<ECB_OneWay>(aesDecryption, iv.ptr ); 
			 break;
		case CRYPT_METHOD::AES_CBC:
			 modeDecryption = new CipherModeFinalTemplate_ExternalCipher<CBC_Decryption>(aesDecryption, iv.ptr );
			 break;
//		case CRYPT_METHOD::AES_CFB:
//#if defined(_IOS)
//             return false;
//#else
//			 modeDecryption = new CFB_Mode_ExternalCipher::Decryption(aesDecryption, iv.ptr );
//#endif
		}
	
		if (!modeDecryption) return false;
		    
		StreamTransformationFilter stfDecryptor(*modeDecryption, new ArraySink( data_out.ptr, data_out.size), padding);
	 
		stfDecryptor.Put( data_inp.ptr, data_inp.size );
		stfDecryptor.MessageEnd();

		delete modeDecryption;
	}

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
namespace CRYPT
{
//-----------------------------------------------------------------------------------------------------------
ECMADecryptor::ECMADecryptor()
{
	bVerify = false;
}

ECMADecryptor::~ECMADecryptor()
{
}

bool ECMADecryptor::SetPassword(std::wstring _password)
{
	bVerify		= false;
	password	= _password;

	if (password.empty()) return false;
	
	if (cryptData.keySize > 64 || cryptData.keySize < 1)  return false;
	if (cryptData.hashSize > 64 || cryptData.hashSize < 1)  return false;
	if (cryptData.blockSize > 64 || cryptData.blockSize < 1)  return false;
	if (cryptData.saltSize > 64 || cryptData.saltSize < 1)  return false;

	buf pPassword		(password);
	buf pSalt			(cryptData.saltValue);
	buf empty			(nullptr, 0, false);

	buf pEncVerInput	(cryptData.encryptedVerifierInput);
	buf pEncVerValue	(cryptData.encryptedVerifierValue);
	
	if (cryptData.bAgile)
	{	
		buf pInputBlockKey ((unsigned char*)encrVerifierHashInputBlockKey, 8);
		buf pValueBlockKey ((unsigned char*)encrVerifierHashValueBlockKey, 8);		
		
		buf verifierInputKey	= GenerateAgileKey( pSalt, pPassword, pInputBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm );
		buf verifierHashKey	= GenerateAgileKey( pSalt, pPassword, pValueBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm );
		
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(verifierInputKey.ptr, cryptData.keySize);
		}
//--------------------------------------------
		buf decryptedVerifierHashInputBytes;		
		DecryptCipher(verifierInputKey, pSalt, pEncVerInput, decryptedVerifierHashInputBytes, cryptData.cipherAlgorithm);
//--------------------------------------------
		buf hashBuf = HashAppend(decryptedVerifierHashInputBytes, empty, cryptData.hashAlgorithm);
//--------------------------------------------
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(verifierHashKey.ptr, cryptData.keySize);
		}		
		buf decryptedVerifierHashBytes;		
		DecryptCipher(verifierHashKey, pSalt, pEncVerValue, decryptedVerifierHashBytes, cryptData.cipherAlgorithm);
//--------------------------------------------
		bVerify	= (decryptedVerifierHashBytes==hashBuf);
	}
	else
	{
		buf verifierKey = GenerateHashKey(pSalt, pPassword, cryptData.hashSize, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);		

		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(verifierKey.ptr, cryptData.keySize);
		}
//--------------------------------------------
		buf decryptedVerifierHashInputBytes;		
		DecryptCipher(verifierKey, pSalt, pEncVerInput, decryptedVerifierHashInputBytes, cryptData.cipherAlgorithm);

	//--------------------------------------------
		buf hashBuf = HashAppend(decryptedVerifierHashInputBytes, empty, cryptData.hashAlgorithm);
	//--------------------------------------------
		
		buf decryptedVerifierHashBytes;		
		DecryptCipher(verifierKey, pSalt, pEncVerValue, decryptedVerifierHashBytes, cryptData.cipherAlgorithm);

		bVerify	= (decryptedVerifierHashBytes == hashBuf);
	}
	return bVerify;
}

bool ECMADecryptor::IsVerify()
{
	return bVerify;
}

void ECMADecryptor::SetCryptData(_ecmaCryptData	& data)
{
	cryptData = data;
}
void ECMADecryptor::Decrypt(char* data	, const size_t size, const unsigned long start_iv_block)
{
	if (!bVerify) return;
	
	unsigned char* data_out = nullptr;
	Decrypt((unsigned char*)data, size, data_out, start_iv_block);
	
	if (data_out)
	{
		memcpy(data, data_out, size);
		delete []data_out;
	}
}
void ECMADecryptor::Decrypt(char* data	, const size_t size, const unsigned long stream_pos, const size_t block_size)
{
	if (!bVerify) return;
//rc4 only
	if (cryptData.cipherAlgorithm != CRYPT_METHOD::RC4) return;
	
	unsigned char* data_out = new unsigned char[size];

	unsigned char*			pnCurrDest	= data_out;
	const unsigned char*	pnCurrSrc	= (unsigned char* )data;
	long					nCurrPos	= stream_pos;
	unsigned short			nBytesLeft	= size;
	
	while(nBytesLeft > 0)
	{
		// initialize codec for current stream position

		int block_index = (nCurrPos / block_size);
		{
			buf pPassword	(password);
			buf pSalt		(cryptData.saltValue);			
			
			buf hashKey = GenerateHashKey(pSalt, pPassword, cryptData.hashSize, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm, block_index);
			
			rc4Decryption.SetKey(hashKey.ptr, cryptData.keySize);
		}

		const long offset = nCurrPos % block_size;
		{//skip
			unsigned char pnDummy[ 1024 ];

			size_t nBytesLeft = offset;
			bool bResult = true;
			while(bResult && (nBytesLeft > 0))
			{
				size_t nBlockLen = nBytesLeft < sizeof(pnDummy) ? nBytesLeft : sizeof(pnDummy);
				rc4Decryption.ProcessData(pnDummy, pnDummy, nBlockLen);
				nBytesLeft -= nBlockLen;
			}
		}
		unsigned short nBlockLeft = static_cast<unsigned short>(block_size - offset);
		unsigned short nDecBytes = nBytesLeft < nBlockLeft ? nBytesLeft : nBlockLeft;
		
		rc4Decryption.ProcessData(pnCurrDest, pnCurrSrc, static_cast<int>(nDecBytes));

		pnCurrDest	+= nDecBytes;
		pnCurrSrc	+= nDecBytes;
		nCurrPos	+= nDecBytes;

		nBytesLeft = nBytesLeft - nDecBytes;
	}	
	if (data_out)
	{
		memcpy(data, data_out, size);
		delete []data_out;
	}
}
bool ECMADecryptor::CheckDataIntegrity(unsigned char* data, int  size)
{
	if (!cryptData.bAgile) return true;

	buf pBlockKey		((unsigned char*)encrKeyValueBlockKey, 8);	
	buf pBlockHmacKey	((unsigned char*)encrDataIntegritySaltBlockKey, 8);	
	buf pBlockHmacValue((unsigned char*)encrDataIntegrityHmacValueBlockKey, 8);	
	
	buf pPassword		(password);
	buf pSalt			(cryptData.saltValue);
	buf empty			(nullptr, 0, false);

	buf pDataSalt		(cryptData.dataSaltValue);
	buf pKeyValue		(cryptData.encryptedKeyValue);
	buf pEncHmacKey	(cryptData.encryptedHmacKey);
	buf pEncHmacValue	(cryptData.encryptedHmacValue);

	buf agileKey = GenerateAgileKey( pSalt, pPassword, pBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);  

	buf secretKey;
	DecryptCipher( agileKey, pSalt, pKeyValue, secretKey, cryptData.cipherAlgorithm);  
//----			
	buf iv1 = HashAppend(pDataSalt, pBlockHmacKey, cryptData.dataHashAlgorithm);
	CorrectHashSize(iv1, cryptData.dataBlockSize, 0x36);
	
	buf iv2 = HashAppend(pDataSalt, pBlockHmacValue, cryptData.dataHashAlgorithm);
	CorrectHashSize(iv2, cryptData.dataBlockSize, 0x36);

	buf pSaltHmac;
	DecryptCipher(secretKey,  iv1, pEncHmacKey, pSaltHmac, cryptData.dataCipherAlgorithm);
	
	buf expected;
	DecryptCipher(secretKey,  iv2, pEncHmacValue, expected, cryptData.dataCipherAlgorithm);

	std::string sData((char*)data, size);
	buf hmac = Hmac(pSaltHmac, cryptData.dataHashAlgorithm, sData);
		
	return (hmac == expected);
}

void ECMADecryptor::Decrypt(unsigned char* data_inp, int size, unsigned char*& data_out, unsigned long start_iv_block)
{
	if (!data_inp || size < 0) return;

	data_out = new unsigned char[size];
	
	buf pPassword	(password);
	buf pSalt		(cryptData.saltValue);
	buf empty		(nullptr, 0, false);

	if (cryptData.bAgile)
	{	
		buf pBlockKey	((unsigned char*)encrKeyValueBlockKey, 8);	
		buf pDataSalt	(cryptData.dataSaltValue);
		buf pKeyValue	(cryptData.encryptedKeyValue);

		buf agileKey = GenerateAgileKey( pSalt, pPassword, pBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);  

		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(agileKey.ptr, cryptData.keySize);
		}
		buf pDecryptedKey;
		DecryptCipher( agileKey, pSalt, pKeyValue, pDecryptedKey, cryptData.cipherAlgorithm);  

		buf iv(cryptData.blockSize, true);

		int i = start_iv_block, sz = 4096, pos = 0;//aes block size = 4096

		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(pDecryptedKey.ptr, cryptData.keySize);
		}
		while (pos < size)
		{
			if (pos + sz > size) 
				sz = size - pos;
			
			buf pIndex((unsigned char*)&i, 4);
			iv = HashAppend(pDataSalt, pIndex, cryptData.dataHashAlgorithm);

			CorrectHashSize(iv, cryptData.dataBlockSize, 0x36);
			
			buf pInp(data_inp + pos, sz, false);
			buf pOut(data_out + pos, sz, false);

			DecryptCipher(pDecryptedKey,  iv, pInp, pOut, cryptData.cipherAlgorithm);
			
			pos += sz; i++;
		}
	}
	else
	{				
		buf hashKey = GenerateHashKey(pSalt, pPassword, cryptData.hashSize, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm, start_iv_block);
		
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{			
			rc4Decryption.SetKey(hashKey.ptr, cryptData.keySize);
		}
	
		buf pInp(data_inp, size, false);
		buf pOut(data_out, size, false);

		DecryptCipher(hashKey, empty, pInp, pOut, cryptData.cipherAlgorithm);
	}
}
//-----------------------------------------------------------------------------------------------------------
void ODFWriteProtect::SetPassword (const std::wstring &password_)
{
	password = password_;
}

void ODFWriteProtect::SetCryptData(_odfWriteProtectData &_data)
{
    data = _data;
}
void ODFWriteProtect::GetCryptData(_odfWriteProtectData &_data)
{
    _data = data;
}

void ODFWriteProtect::Generate()
{
    //generate salt
    RandomPool prng;
    SecByteBlock seed_salt(16);
    OS_GenerateRandomBlock(false, seed_salt, seed_salt.size());
    if (prng.CanIncorporateEntropy())
    {
        prng.IncorporateEntropy(seed_salt, seed_salt.size());
    }

    std::string passw_ansi = std::string(password.begin(), password.end());
    buf pPassword	(passw_ansi);
    buf pSalt		(seed_salt.data(), seed_salt.size());

    buf pHash = GenerateOdfKey(pSalt, pPassword, 16, data.spinCount, CRYPT_METHOD::None);

    data.saltValue = std::string((char*)pSalt.ptr, pSalt.size);
    data.hashValue = std::string((char*)pHash.ptr, pHash.size);
}
bool ODFWriteProtect::Verify()
{
    std::string passw_ansi = std::string(password.begin(), password.end());

    buf pPassword	(passw_ansi);
    buf pHash (data.hashValue);

    if (data.hashAlgorithm == CRYPT_METHOD::PBKDF2)
    {
        buf pSalt (data.saltValue);

        buf pHashTest = GenerateOdfKey(pSalt, pPassword, 16, data.spinCount, CRYPT_METHOD::None);

        return (pHashTest == pHash);
    }
    else
    {
        buf empty (nullptr, 0, false);

        buf pHashTest = HashAppend(empty, pPassword, data.hashAlgorithm);

        return (pHashTest == pHash);
    }
}
//----------------------------------------------------------------------------------------------------------
void ECMAWriteProtect::SetPassword (const std::wstring &password_)
{
	password = password_;
}
void ECMAWriteProtect::SetCryptData(_ecmaWriteProtectData &_data)
{
	data = _data;
}
void ECMAWriteProtect::GetCryptData(_ecmaWriteProtectData &_data)
{
	_data = data;
}
void ECMAWriteProtect::Generate()
{
	//generate salt
	RandomPool prng;
	SecByteBlock seed_salt(16);
	OS_GenerateRandomBlock(false, seed_salt, seed_salt.size());
	if (prng.CanIncorporateEntropy())
	{
		prng.IncorporateEntropy(seed_salt, seed_salt.size());
	}
	buf pPassword	(password);
	buf empty		(nullptr, 0, false);		
	buf pSalt		(seed_salt.data(), seed_salt.size());

	buf pHashBuf = HashAppend(pSalt, pPassword, data.hashAlgorithm);
		
	for (_UINT32 i = 0; i < data.spinCount; i++)
	{
        buf iterator((unsigned char*)&i, 4, false);
        pHashBuf = HashAppend(pHashBuf, iterator, data.hashAlgorithm);
	}

	data.saltValue = std::string((char*)pSalt.ptr, pSalt.size);
	data.hashValue = std::string((char*)pHashBuf.ptr, pHashBuf.size);
}
bool ECMAWriteProtect::VerifyWrike()
{
	std::string p = std::string(password.begin(), password.end());

    unsigned int wPasswordHash = 0;
  
    const char* pch = p.c_str() + p.length();
    while (pch-- != p.c_str())
    {
        wPasswordHash = ((wPasswordHash >> 14) & 0x01) | 
                        ((wPasswordHash << 1) & 0x7fff);
        wPasswordHash ^= *pch;
    }
 
    wPasswordHash = ((wPasswordHash >> 14) & 0x01) | 
                    ((wPasswordHash << 1) & 0x7fff);
 
    wPasswordHash ^= (0x8000 | ('N' << 8) | 'K');
    wPasswordHash ^= p.length();

	std::stringstream sstream;
	sstream << boost::format("%4.4X") % wPasswordHash;
	std::string sPasswordHash(sstream.str());

	return data.hashValue == sPasswordHash;
}
bool ECMAWriteProtect::Verify()
{
	buf pPassword	(password);
	buf empty		(nullptr, 0, false);		
	buf pSalt		(data.saltValue);
	buf pHash		(data.hashValue);

	buf pHashTest = HashAppend(pSalt, pPassword, data.hashAlgorithm);
		
	for (_UINT32 i = 0; i < data.spinCount; i++)
	{
        buf iterator((unsigned char*)&i, 4, false);
        pHashTest = HashAppend(pHashTest, iterator, data.hashAlgorithm);
	}
	return (pHashTest == pHash);
}
//-----------------------------------------------------------------------------------------------------------
ECMAEncryptor::ECMAEncryptor()
{
}

void ECMAEncryptor::SetPassword(std::wstring _password)
{
	password = _password;

//---------
    RandomPool prng;
	
	//generate salt
    SecByteBlock seed_salt(cryptData.saltSize);
    OS_GenerateRandomBlock(false, seed_salt, seed_salt.size());
	if (prng.CanIncorporateEntropy())
	{
		prng.IncorporateEntropy(seed_salt, seed_salt.size());
	}
    SecByteBlock seed_datasalt(cryptData.saltSize);
    OS_GenerateRandomBlock(false, seed_datasalt, seed_datasalt.size());
	if (prng.CanIncorporateEntropy())
	{
		prng.IncorporateEntropy(seed_datasalt, seed_datasalt.size());
	}
	//generate a key
    SecByteBlock seed_key(cryptData.keySize);
    OS_GenerateRandomBlock(false, seed_key, seed_key.size());
	if (prng.CanIncorporateEntropy())
	{
		prng.IncorporateEntropy(seed_key, seed_key.size());
	}
	//generate a test
    SecByteBlock seed_verify(cryptData.saltSize);
    OS_GenerateRandomBlock(false, seed_verify, seed_verify.size());
	if (prng.CanIncorporateEntropy())
	{
		prng.IncorporateEntropy(seed_verify, seed_verify.size());
	}
//---------
	buf pPassword		(password);
	buf empty			(nullptr, 0, false);
		
	buf pSalt			(seed_salt.data(), seed_salt.size());
	buf pDataSalt		(seed_datasalt.data(), seed_datasalt.size());
	buf pDecryptedKey	(seed_key.data(), seed_key.size());
	
//------------------------------------------------------------------------------------------------
	if (cryptData.bAgile)
	{
		buf pBlockKey		((unsigned char*)encrKeyValueBlockKey, 8);	
		buf pInputBlockKey ((unsigned char*)encrVerifierHashInputBlockKey, 8);
		buf pValueBlockKey ((unsigned char*)encrVerifierHashValueBlockKey, 8);
		
		buf agileKey = GenerateAgileKey( pSalt, pPassword, pBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);  

		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Encryption.SetKey(agileKey.ptr, cryptData.keySize);
		}
		buf pKeyValue;
		EncryptCipher( agileKey, pSalt, pDecryptedKey, pKeyValue, cryptData.cipherAlgorithm);  
		
	//--------------------------------------------
		buf decryptedVerifierHashInputBytes(seed_verify.data(), seed_verify.size());
		buf verifierInputKey = GenerateAgileKey( pSalt, pPassword, pInputBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm );
		
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Encryption.SetKey(verifierInputKey.ptr, cryptData.keySize);
		}
		buf pEncVerInput;
		EncryptCipher( verifierInputKey, pSalt, decryptedVerifierHashInputBytes, pEncVerInput, cryptData.cipherAlgorithm);  
	//--------------------------------------------

		buf decryptedVerifierHashBytes = HashAppend(decryptedVerifierHashInputBytes, empty, cryptData.hashAlgorithm);
		buf verifierHashKey = GenerateAgileKey(pSalt, pPassword, pValueBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);
		
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Encryption.SetKey(verifierHashKey.ptr, cryptData.keySize);
		}
		else if (decryptedVerifierHashBytes.size % PADDING_SIZE != 0) 
		{
			CorrectHashSize(decryptedVerifierHashBytes, (decryptedVerifierHashBytes.size / PADDING_SIZE + 1) * PADDING_SIZE, 0);
		}
		buf pEncVerValue;
		EncryptCipher( verifierHashKey, pSalt, decryptedVerifierHashBytes, pEncVerValue, cryptData.cipherAlgorithm);  

		cryptData.saltValue					= std::string((char*)pSalt.ptr, pSalt.size);
		cryptData.dataSaltValue				= std::string((char*)pDataSalt.ptr, pDataSalt.size);
		cryptData.encryptedKeyValue			= std::string((char*)pKeyValue.ptr, pKeyValue.size);
		cryptData.encryptedVerifierInput	= std::string((char*)pEncVerInput.ptr, pEncVerInput.size);
		cryptData.encryptedVerifierValue	= std::string((char*)pEncVerValue.ptr, pEncVerValue.size);
	}
	else
	{
		buf verifierKey = GenerateHashKey(pSalt, pPassword, cryptData.hashSize, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);		

		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{			
			rc4Encryption.SetKey(verifierKey.ptr, cryptData.keySize);
		}	
		buf decryptedVerInput(seed_verify.data(), seed_verify.size());
		
		buf pEncVerInput;
		EncryptCipher( verifierKey, pSalt, decryptedVerInput, pEncVerInput, cryptData.cipherAlgorithm);  
		
		buf hashBuf = HashAppend(decryptedVerInput, empty, cryptData.hashAlgorithm);
		if (cryptData.cipherAlgorithm != CRYPT_METHOD::RC4 && hashBuf.size % PADDING_SIZE != 0) 
		{
			CorrectHashSize(hashBuf, (hashBuf.size / PADDING_SIZE + 1) * PADDING_SIZE, 0);
		}
		
		buf pEncVerValue;
		EncryptCipher( verifierKey, pSalt, hashBuf, pEncVerValue, cryptData.cipherAlgorithm);  

		cryptData.saltValue					= std::string((char*)pSalt.ptr, pSalt.size);
		cryptData.encryptedVerifierInput	= std::string((char*)pEncVerInput.ptr, pEncVerInput.size);
		cryptData.encryptedVerifierValue	= std::string((char*)pEncVerValue.ptr, pEncVerValue.size);
	}
}

void ECMAEncryptor::SetCryptData(_ecmaCryptData	& data)
{
	cryptData = data;
}

void ECMAEncryptor::GetCryptData(_ecmaCryptData &data)
{
	data = cryptData;
}

void ECMAEncryptor::UpdateDataIntegrity(unsigned char* data, int  size)
{
	if (!cryptData.bAgile) return;

	buf pBlockKey		((unsigned char*)encrKeyValueBlockKey, 8);	
	buf pBlockHmacKey	((unsigned char*)encrDataIntegritySaltBlockKey, 8);	
	buf pBlockHmacValue((unsigned char*)encrDataIntegrityHmacValueBlockKey, 8);	
	
	buf pPassword		(password);
	buf pSalt			(cryptData.saltValue);
	buf empty			(nullptr, 0, false);

	buf pDataSalt		(cryptData.dataSaltValue);
	buf pKeyValue		(cryptData.encryptedKeyValue);
	
//----
	buf agileKey = GenerateAgileKey( pSalt, pPassword, pBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);  

	if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
	{
		rc4Encryption.SetKey(agileKey.ptr, cryptData.keySize);
	}

	buf secretKey;
	DecryptCipher( agileKey, pSalt, pKeyValue, secretKey, cryptData.cipherAlgorithm);  
	
	buf iv1 = HashAppend(pDataSalt, pBlockHmacKey, cryptData.hashAlgorithm);
	CorrectHashSize(iv1, cryptData.blockSize, 0x36);
	
	buf iv2 = HashAppend(pDataSalt, pBlockHmacValue, cryptData.hashAlgorithm);
	CorrectHashSize(iv2, cryptData.blockSize, 0x36);

//----
    RandomPool prng;
    SecByteBlock seed(cryptData.hashSize);

    OS_GenerateRandomBlock(false, seed, seed.size());
	prng.IncorporateEntropy(seed, seed.size());
	
	buf pSaltHmac(seed.data(), seed.size());

	std::string sData((char*)data, size);
	buf hmac = Hmac(pSaltHmac, cryptData.hashAlgorithm, sData);

	if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
	{
		rc4Encryption.SetKey(secretKey.ptr, cryptData.keySize);
	}
	if (pSaltHmac.size % PADDING_SIZE != 0) 
	{
		CorrectHashSize(pSaltHmac, (pSaltHmac.size / PADDING_SIZE + 1) * PADDING_SIZE, 0);
	}
	buf pEncHmacKey;
	EncryptCipher(secretKey,  iv1, pSaltHmac, pEncHmacKey, cryptData.cipherAlgorithm);

	if (hmac.size % PADDING_SIZE != 0) 
	{
		CorrectHashSize(hmac, (hmac.size / PADDING_SIZE + 1) * PADDING_SIZE, 0);
	}
	buf pEncHmacValue;
	EncryptCipher(secretKey,  iv2, hmac, pEncHmacValue, cryptData.cipherAlgorithm);

	cryptData.encryptedHmacKey		= std::string((char*)pEncHmacKey.ptr, pEncHmacKey.size);
	cryptData.encryptedHmacValue	= std::string((char*)pEncHmacValue.ptr, pEncHmacValue.size);
}

int ECMAEncryptor::Encrypt(unsigned char* data_inp_ptr, int size, unsigned char*& data_out_ptr)
{
	data_out_ptr = nullptr;
	
	buf pPassword	(password);
	buf pSalt		(cryptData.saltValue);
	buf empty		(nullptr, 0, false);

	int size_out = size;

	if (cryptData.cipherAlgorithm != CRYPT_METHOD::RC4 && size_out % PADDING_SIZE != 0) 
		size_out = (size_out / PADDING_SIZE + 1) * PADDING_SIZE;
	
	data_out_ptr = new unsigned char[size_out +	PADDING_SIZE]; // real size + padding + size for realsize
	
	_UINT64 nSize = size;
	memcpy(data_out_ptr, (unsigned char*)&nSize, 8);

	unsigned char* data_inp = data_inp_ptr;
	unsigned char* data_out = data_out_ptr + 8;
	
	int result_size_out = 0;

	if (cryptData.bAgile)
	{	
		buf pBlockKey	((unsigned char*)encrKeyValueBlockKey, 8);	
		buf pDataSalt	(cryptData.dataSaltValue);
		buf pKeyValue	(cryptData.encryptedKeyValue);
		
	//------------------------------------------------------------------------------------------------
		buf agileKey = GenerateAgileKey( pSalt, pPassword, pBlockKey, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);  

		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(agileKey.ptr, cryptData.keySize);
		}
		buf pDecryptedKey;
		DecryptCipher( agileKey, pSalt, pKeyValue, pDecryptedKey, cryptData.cipherAlgorithm);  
		
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Encryption.SetKey(pDecryptedKey.ptr, cryptData.keySize);
		}
	//-------------------------------------------------------------------------------------------------
		buf iv(cryptData.blockSize, true);

		int i = 0, sz = 4096, enc_size = 0;

		while (enc_size < size)
		{
			if (enc_size + sz > size) 
			{
				sz = size - enc_size;
			}
			
			buf pIndex((unsigned char*)&i, 4);
			iv = HashAppend(pDataSalt, pIndex, cryptData.hashAlgorithm);

			CorrectHashSize(iv, cryptData.blockSize, 0x36);
			
			if (sz < 4096)
			{
				buf pInp(4096); 
					memcpy(pInp.ptr, data_inp, sz );
					pInp.size = sz;
				buf pOut(4096);

				EncryptCipher(pDecryptedKey,  iv, pInp, pOut, cryptData.cipherAlgorithm);
				
				if (sz % PADDING_SIZE != 0) 
					sz = (sz / PADDING_SIZE + 1) * PADDING_SIZE;
				
				memcpy(data_out, pOut.ptr, sz);

			}
			else
			{
				buf pInp(data_inp, sz, false);
				buf pOut(data_out, sz, false);
				
				EncryptCipher(pDecryptedKey,  iv, pInp, pOut, cryptData.cipherAlgorithm);
			}
			data_inp += sz;
			data_out += sz;

			enc_size += sz; i++;
		}
		result_size_out = enc_size + 8;
	}
	else
	{
		buf hashKey = GenerateHashKey(pSalt, pPassword, cryptData.hashSize, cryptData.keySize, cryptData.spinCount, cryptData.hashAlgorithm);
		
		if (cryptData.cipherAlgorithm == CRYPT_METHOD::RC4)
		{
			rc4Decryption.SetKey(hashKey.ptr, cryptData.keySize);
			rc4Encryption.SetKey(hashKey.ptr, cryptData.keySize);
		}
	
		buf pInp(data_inp, size, false);
		buf pOut(data_out, size_out, false);

		EncryptCipher(hashKey, empty, pInp, pOut, cryptData.cipherAlgorithm/*, StreamTransformationFilter::ZEROS_PADDING*/);	
	
		result_size_out = size_out + 8;
	}
	return result_size_out;
}
//-----------------------------------------------------------------------------------------------------------
ODFDecryptor::ODFDecryptor()
{
	bVerify = false;
}

void ODFDecryptor::SetCryptData(_odfCryptData	& data)
{
	cryptData = data;
}

bool ODFDecryptor::Decrypt(const std::wstring & wspassword, unsigned char* data_inp, int size_inp, unsigned char*& data_out, int &size_out)
{
	bVerify = false;

	std::string password = NSFile::CUtf8Converter::GetUtf8StringFromUnicode(wspassword);

	buf pPassword	(password);
	buf pSalt		(cryptData.saltValue);
	buf ivi		(cryptData.initializationVector);
	buf empty		(nullptr, 0, false);
	
	buf pKey = GenerateOdfKey(pSalt, pPassword, cryptData.keySize, cryptData.spinCount, cryptData.start_hashAlgorithm);

	buf pInp(data_inp, size_inp, false);
	buf pOut(size_inp);	

	bool bPadding = false;
	
	if ((cryptData.cipherAlgorithm == CRYPT_METHOD::AES_CBC) && (size_inp % PADDING_SIZE != 0))
	{
		bPadding = true;
	}

	if (!DecryptCipher(pKey, ivi, pInp, pOut, cryptData.cipherAlgorithm, bPadding ? StreamTransformationFilter::W3C_PADDING : 
																							StreamTransformationFilter::NO_PADDING))
	{
		return false;
	}

	buf pChecksum	(cryptData.checksum);
	
	buf pOutLimit(pOut.ptr, (std::min)(cryptData.checksum_size, pOut.size), false);
	buf pOutHash = HashAppend(pOutLimit, empty, cryptData.checksum_hashAlgorithm);

	bVerify	= (pChecksum == pOutHash);

	if (!bVerify && pOut.size < cryptData.checksum_size + 16)
	{
		for (int i = 0; !bVerify && i < pOut.size; i++)
		{
			buf pOutLimit2(pOut.size - i, true);
			memcpy(pOutLimit2.ptr, pOut.ptr, pOut.size - i);

			buf pOutHash2 = HashAppend(pOutLimit2, empty, cryptData.checksum_hashAlgorithm);

			bVerify	= (pChecksum == pOutHash2);
		}
	}

	try
	{
		if (bVerify)
		{
			data_out = new unsigned char[size_out + 1024];

			Inflator inflator(new ArraySink(data_out, size_out + 1024));

			inflator.Put(pOut.ptr, pOut.size);
			inflator.MessageEnd();

			bVerify = true;
		}
	}
	catch(...)
	{
		return false;
	}
	return bVerify;
}

//-----------------------------------------------------------------------------------------------------------
int ODFEncryptor::Encrypt (const std::wstring & wspassword, unsigned char* data, int  size, unsigned char*& data_out)
{
	std::string password = NSFile::CUtf8Converter::GetUtf8StringFromUnicode(wspassword);

	buf pPassword	(password);
	buf pSalt		(cryptData.saltValue);
	buf ivi		(cryptData.initializationVector);
	buf empty		(nullptr, 0, false);

	buf pKey = GenerateOdfKey(pSalt, pPassword, cryptData.keySize, cryptData.spinCount, cryptData.start_hashAlgorithm);
//---------
	buf data_deflate(size + 1024); 
	ArraySink *sink = new ArraySink(data_deflate.ptr, data_deflate.size + 1024);

	Deflator deflator(sink, Deflator::DEFAULT_DEFLATE_LEVEL, Deflator::DEFAULT_LOG2_WINDOW_SIZE, true);
	deflator.Put2(data, size, 1, true);
	data_deflate.size = sink->TotalPutLength();
//---------	
	buf pOutLimit(data_deflate.ptr, (std::min)(cryptData.checksum_size, data_deflate.size), false);
	buf pChecksum = HashAppend(pOutLimit, empty, cryptData.checksum_hashAlgorithm);

	cryptData.checksum = std::string((char*)pChecksum.ptr, pChecksum.size);
//---------	
	int size_out = data_deflate.size;

	bool bPadding = (cryptData.cipherAlgorithm == CRYPT_METHOD::AES_CBC);
	
	if (bPadding && (size_out % PADDING_SIZE != 0))
		size_out = (size_out / PADDING_SIZE + 1) * PADDING_SIZE;

	data_out = new unsigned char[size_out]; 	
	
	buf pOut (data_out, size_out, false);	

	EncryptCipher(pKey,  ivi, data_deflate, pOut, cryptData.cipherAlgorithm, bPadding ? StreamTransformationFilter::W3C_PADDING :
																						StreamTransformationFilter::NO_PADDING);
	return size_out;
}

void ODFEncryptor::SetCryptData(_odfCryptData	&data)
{
	cryptData = data;

    RandomPool prng;
	
	if (cryptData.saltValue.empty())
	{
        SecByteBlock seed_salt(16);
        OS_GenerateRandomBlock(false, seed_salt, seed_salt.size());
		if (prng.CanIncorporateEntropy())
		{
			prng.IncorporateEntropy(seed_salt, seed_salt.size());
		}
		cryptData.saltValue = std::string((char*)seed_salt.data(), seed_salt.size());
	}
	
	if (cryptData.initializationVector.empty())
	{
        SecByteBlock start_vector(16);
        OS_GenerateRandomBlock(false, start_vector, start_vector.size());
		if (prng.CanIncorporateEntropy())
		{
			prng.IncorporateEntropy(start_vector, start_vector.size());
		}
		cryptData.initializationVector = std::string((char*)start_vector.data(), start_vector.size());
	}
}

void ODFEncryptor::GetCryptData(_odfCryptData &data)
{
	data = cryptData;
}
}
