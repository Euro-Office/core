/*
 * (c) Copyright Ascensio System SIA 2010-2019
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <string>
  
#include "./../common/common_openssl.h"

TEST_SUITE( "OpenSSL" )
{
    TEST_CASE( "SHA-256" )
    {
        std::string sTestHashString = "knoejnrgijwenrgiojwnergjiwnerigjnwerojgnweorigjn";
        unsigned int data_len = 0;
        unsigned char* data = NSOpenSSL::GetHash((unsigned char*)sTestHashString.c_str(), (unsigned int)sTestHashString.length(), OPENSSL_HASH_ALG_SHA256, data_len);
        std::string sResult = NSOpenSSL::Serialize(data, data_len, OPENSSL_SERIALIZE_TYPE_HEX);

        NSOpenSSL::openssl_free(data);

        const std::string sExpectedResult = "913DD5544D5C726A40D551ACE85B29ABBBD4E2ADF2F4DA5BD07CEC07680CF5CD";
        CHECK( sResult == sExpectedResult );
    }

    TEST_CASE( "RSA" )
    {
        unsigned char* publicKey = NULL;
        unsigned char* privateKey = NULL;
        bool bRes = NSOpenSSL::RSA_GenerateKeys(publicKey, privateKey);

        std::string sPublic((char*)publicKey);
        std::string sPrivate((char*)privateKey);

        NSOpenSSL::openssl_free(publicKey);
        NSOpenSSL::openssl_free(privateKey);

        const std::string sMessage = "Hello world";

        unsigned char* message_crypt = NULL;
        unsigned int message_crypt_len = 0;
        bool bEncrypt = NSOpenSSL::RSA_EncryptPublic((unsigned char*)sPublic.c_str(), (const unsigned char*)sMessage.c_str(), (unsigned int)sMessage.length(), message_crypt, message_crypt_len);

        unsigned char* message_decrypt = NULL;
        unsigned int message_decrypt_len = 0;

        bool bDecrypt = NSOpenSSL::RSA_DecryptPrivate((unsigned char*)sPrivate.c_str(), message_crypt, message_crypt_len, message_decrypt, message_decrypt_len);

        std::string sMessageOut((char*)message_decrypt, message_decrypt_len);

        NSOpenSSL::openssl_free(message_crypt);
        NSOpenSSL::openssl_free(message_decrypt);

        CHECK( sMessageOut == sMessage );
    }

    TEST_CASE( "AES" )
    {
        const std::string password = "{PASSWORD}";
        const std::string message = "{MESSAGE}";
        std::string message_crypted = "";
        std::string message_decrypted = "";

        NSOpenSSL::AES_Encrypt_desktop(password, message, message_crypted);
        NSOpenSSL::AES_Decrypt_desktop(password, message_crypted, message_decrypted);

        CHECK( message_decrypted == message );
    }
}
