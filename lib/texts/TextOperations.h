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

/// Namespace that provides utilities for unicode support (UTF-8)
namespace TextOperations
{
	/// returns 32-bit UTF codepoint for UTF-8 character symbol
	uint32_t DLL_LINKAGE getUnicodeCodepoint(const char *data, size_t maxSize);

	/// returns 32-bit UTF codepoint for character symbol in selected single-byte encoding
	uint32_t DLL_LINKAGE getUnicodeCodepoint(char data, const std::string & encoding );

	/// returns length (in bytes) of UTF-8 character starting from specified character
	size_t DLL_LINKAGE getUnicodeCharacterSize(char firstByte);

	/// test if character is a valid UTF-8 symbol
	/// maxSize - maximum number of bytes this symbol may consist from ( = remainder of string)
	bool DLL_LINKAGE isValidUnicodeCharacter(const char * character, size_t maxSize);

	/// returns true if text contains valid ASCII-string
	/// Note that since UTF-8 extends ASCII, any ASCII string is also UTF-8 string
	bool DLL_LINKAGE isValidASCII(const std::string & text);
	bool DLL_LINKAGE isValidASCII(const char * data, size_t size);

	/// test if text contains valid UTF-8 sequence
	bool DLL_LINKAGE isValidUnicodeString(const std::string & text);
	bool DLL_LINKAGE isValidUnicodeString(const char * data, size_t size);

	/// converts text to UTF-8 from specified encoding or from one specified in settings
	std::string DLL_LINKAGE toUnicode(const std::string & text, const std::string & encoding);

	/// converts text from unicode to specified encoding or to one specified in settings
	/// NOTE: usage of these functions should be avoided if possible
	std::string DLL_LINKAGE fromUnicode(const std::string & text, const std::string & encoding);

	///delete specified amount of UTF-8 characters from right
	DLL_LINKAGE void trimRightUnicode(std::string & text, size_t amount = 1);

	/// give back amount of unicode characters
	size_t DLL_LINKAGE getUnicodeCharactersCount(const std::string & text);

	/// converts number into string using metric system prefixes, e.g. 'k' or 'M' to keep resulting strings within specified size
	/// Note that resulting string may have more symbols than digits: minus sign and prefix symbol
	template<typename Arithmetic>
	inline std::string formatMetric(Arithmetic number, int maxDigits);

	/// replaces all symbols that normally need escaping with appropriate escape sequences
	std::string escapeString(std::string input);

	/// get formatted DateTime depending on the language selected
	DLL_LINKAGE std::string getFormattedDateTimeLocal(std::time_t dt);

	/// get formatted current DateTime depending on the language selected
	/// timeOffset - optional parameter to modify current time by specified time in seconds
	DLL_LINKAGE std::string getCurrentFormattedDateTimeLocal(std::chrono::seconds timeOffset = {});

	/// get formatted time (without date)
	DLL_LINKAGE std::string getFormattedTimeLocal(std::time_t dt);

	/// get formatted time (without date)
	/// timeOffset - optional parameter to modify current time by specified time in seconds
	DLL_LINKAGE std::string getCurrentFormattedTimeLocal(std::chrono::seconds timeOffset = {});
};

template<typename Arithmetic>
inline std::string TextOperations::formatMetric(Arithmetic number, int maxDigits)
{
	Arithmetic max = std::pow(10, maxDigits);
	if (std::abs(number) < max)
		return std::to_string(number);

	std::string symbols = " kMGTPE";
	auto iter = symbols.begin();

	while (std::abs(number) >= max)
	{
		number /= 1000;
		iter++;

		assert(iter != symbols.end());//should be enough even for int64
	}
	return std::to_string(number) + *iter;
}

VCMI_LIB_NAMESPACE_END
