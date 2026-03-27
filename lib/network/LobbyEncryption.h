/*
 * LobbyEncryption.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../Global.h"

/// RSA-4096 OAEP-SHA256 encryption utilities for the lobby protocol.
/// The server holds the private key; clients encrypt with the matching public key.
/// Keys are stored in PEM format.
class DLL_LINKAGE LobbyEncryption
{
public:
	/// Encrypt plaintext with an RSA public key (PEM). Returns a base64-encoded ciphertext.
	static std::string encrypt(const std::string & plaintext, const std::string & publicKeyPem);

	/// Decrypt a base64-encoded ciphertext with an RSA private key (PEM).
	static std::string decrypt(const std::string & ciphertext, const std::string & privateKeyPem);

	/// Generate an ephemeral RSA-2048 key pair. Returns {publicKeyPem, privateKeyPem}.
	/// Used by clients to receive encrypted replies from the server without managing key files.
	static std::pair<std::string, std::string> generateKeyPair();
};
