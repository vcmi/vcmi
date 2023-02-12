/*
 * TextOperations.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Namespace that provides utilites for unicode support (UTF-8)
namespace Unicode
{
	/// evaluates size of UTF-8 character
	size_t DLL_LINKAGE getCharacterSize(char firstByte);

	/// test if character is a valid UTF-8 symbol
	/// maxSize - maximum number of bytes this symbol may consist from ( = remainer of string)
	bool DLL_LINKAGE isValidCharacter(const char * character, size_t maxSize);

	/// test if text contains ASCII-string (no need for unicode conversion)
	bool DLL_LINKAGE isValidASCII(const std::string & text);
	bool DLL_LINKAGE isValidASCII(const char * data, size_t size);

	/// test if text contains valid UTF-8 sequence
	bool DLL_LINKAGE isValidString(const std::string & text);
	bool DLL_LINKAGE isValidString(const char * data, size_t size);

	/// converts text to unicode from specified encoding or from one specified in settings
	std::string DLL_LINKAGE toUnicode(const std::string & text);
	std::string DLL_LINKAGE toUnicode(const std::string & text, const std::string & encoding);

	/// converts text from unicode to specified encoding or to one specified in settings
	/// NOTE: usage of these functions should be avoided if possible
	std::string DLL_LINKAGE fromUnicode(const std::string & text);
	std::string DLL_LINKAGE fromUnicode(const std::string & text, const std::string & encoding);

	///delete (amount) UTF characters from right
	DLL_LINKAGE void trimRight(std::string & text, size_t amount = 1);
};

VCMI_LIB_NAMESPACE_END
