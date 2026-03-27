/*
 * LobbyEncryption.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LobbyEncryption.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

static std::string opensslLastError()
{
	char buf[256];
	ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
	return buf;
}

static std::string base64Encode(const std::vector<unsigned char> & data)
{
	BIO * b64 = BIO_new(BIO_f_base64());
	BIO * mem = BIO_new(BIO_s_mem());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO_push(b64, mem);
	BIO_write(b64, data.data(), static_cast<int>(data.size()));
	BIO_flush(b64);

	BUF_MEM * buf;
	BIO_get_mem_ptr(mem, &buf);
	std::string result(buf->data, buf->length);
	BIO_free_all(b64);
	return result;
}

static std::vector<unsigned char> base64Decode(const std::string & encoded)
{
	std::vector<unsigned char> out(encoded.size());
	BIO * b64 = BIO_new(BIO_f_base64());
	BIO * mem = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO_push(b64, mem);
	int len = BIO_read(b64, out.data(), static_cast<int>(encoded.size()));
	BIO_free_all(b64);

	if(len <= 0)
		throw std::runtime_error("LobbyEncryption: base64 decode failed");

	out.resize(len);
	return out;
}

std::string LobbyEncryption::encrypt(const std::string & plaintext, const std::string & publicKeyPem)
{
	BIO * bio = BIO_new_mem_buf(publicKeyPem.data(), static_cast<int>(publicKeyPem.size()));
	EVP_PKEY * pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
	BIO_free(bio);

	if(!pkey)
		throw std::runtime_error("LobbyEncryption: failed to load public key: " + opensslLastError());

	EVP_PKEY_CTX * ctx = EVP_PKEY_CTX_new(pkey, nullptr);
	EVP_PKEY_free(pkey);

	EVP_PKEY_encrypt_init(ctx);
	EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
	EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());
	EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256());

	size_t outlen = 0;
	EVP_PKEY_encrypt(ctx, nullptr, &outlen,
		reinterpret_cast<const unsigned char *>(plaintext.data()), plaintext.size());

	std::vector<unsigned char> out(outlen);
	if(EVP_PKEY_encrypt(ctx, out.data(), &outlen,
		reinterpret_cast<const unsigned char *>(plaintext.data()), plaintext.size()) <= 0)
	{
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("LobbyEncryption: encryption failed: " + opensslLastError());
	}

	EVP_PKEY_CTX_free(ctx);
	out.resize(outlen);
	return base64Encode(out);
}

std::string LobbyEncryption::decrypt(const std::string & ciphertext, const std::string & privateKeyPem)
{
	auto cipherBytes = base64Decode(ciphertext);

	BIO * bio = BIO_new_mem_buf(privateKeyPem.data(), static_cast<int>(privateKeyPem.size()));
	EVP_PKEY * pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
	BIO_free(bio);

	if(!pkey)
		throw std::runtime_error("LobbyEncryption: failed to load private key: " + opensslLastError());

	EVP_PKEY_CTX * ctx = EVP_PKEY_CTX_new(pkey, nullptr);
	EVP_PKEY_free(pkey);

	EVP_PKEY_decrypt_init(ctx);
	EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
	EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());
	EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256());

	size_t outlen = 0;
	EVP_PKEY_decrypt(ctx, nullptr, &outlen, cipherBytes.data(), cipherBytes.size());

	std::vector<unsigned char> out(outlen);
	if(EVP_PKEY_decrypt(ctx, out.data(), &outlen, cipherBytes.data(), cipherBytes.size()) <= 0)
	{
		EVP_PKEY_CTX_free(ctx);
		throw std::runtime_error("LobbyEncryption: decryption failed: " + opensslLastError());
	}

	EVP_PKEY_CTX_free(ctx);
	out.resize(outlen);
	return std::string(reinterpret_cast<const char *>(out.data()), outlen);
}

std::pair<std::string, std::string> LobbyEncryption::generateKeyPair()
{
	EVP_PKEY * pkey = EVP_RSA_gen(2048);
	if(!pkey)
		throw std::runtime_error("LobbyEncryption: failed to generate key pair: " + opensslLastError());

	// Public key
	BIO * pubBio = BIO_new(BIO_s_mem());
	PEM_write_bio_PUBKEY(pubBio, pkey);
	BUF_MEM * pubBuf;
	BIO_get_mem_ptr(pubBio, &pubBuf);
	std::string pubKey(pubBuf->data, pubBuf->length);
	BIO_free(pubBio);

	// Private key
	BIO * privBio = BIO_new(BIO_s_mem());
	PEM_write_bio_PrivateKey(privBio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
	BUF_MEM * privBuf;
	BIO_get_mem_ptr(privBio, &privBuf);
	std::string privKey(privBuf->data, privBuf->length);
	BIO_free(privBio);

	EVP_PKEY_free(pkey);
	return {pubKey, privKey};
}
