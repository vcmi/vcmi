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

#include <boost/lexical_cast.hpp>

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
	bool DLL_LINKAGE isValidUnicodeString(std::string_view text);
	bool DLL_LINKAGE isValidUnicodeString(const char * data, size_t size);

	/// converts text to UTF-8 from specified encoding or from one specified in settings
	std::string DLL_LINKAGE toUnicode(const std::string & text, const std::string & encoding);

	/// converts text from unicode to specified encoding or to one specified in settings
	/// NOTE: usage of these functions should be avoided if possible
	std::string DLL_LINKAGE fromUnicode(const std::string & text, const std::string & encoding);

	///delete specified amount of UTF-8 characters from right
	DLL_LINKAGE void trimRightUnicode(std::string & text, size_t amount = 1);

	/// give back amount of unicode characters
	size_t DLL_LINKAGE getUnicodeCharactersCount(std::string_view text);

	/// converts number into string using metric system prefixes, e.g. 'k' or 'M' to keep resulting strings within specified size
	/// Note that resulting string may have more symbols than digits: minus sign and prefix symbol
	template<typename Arithmetic>
	inline std::string formatMetric(Arithmetic number, int maxDigits);

	template<typename Arithmetic>
	inline Arithmetic parseMetric(const std::string &text);

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

	/// Algorithm for detection of typos in words
	/// Determines how 'different' two strings are - how many changes must be done to turn one string into another one
	/// https://en.wikipedia.org/wiki/Levenshtein_distance#Iterative_with_two_matrix_rows
	DLL_LINKAGE int getLevenshteinDistance(std::string_view s, std::string_view t);

	/// Compares two strings using locale-aware collation based on the selected game language.
	DLL_LINKAGE bool compareLocalizedStrings(std::string_view str1, std::string_view str2);

	/// Check if texts have similarity when typing into search boxes
	/// 0 -> Exact match or starts with typed-in text, 1 -> Close match or substring match, 
	/// other values = Levenshtein distance, returns std::nullopt for unrelated word (bad match).
	DLL_LINKAGE std::optional<int> textSearchSimilarityScore(const std::string & s, const std::string & t);

	/// This function is mainly used to avoid garbled text when reading or writing files
	/// with non-ASCII (e.g. Chinese) characters in the path, especially on Windows.
	/// Call this before passing the path to file I/O functions that take std::string.
	DLL_LINKAGE std::string filesystemPathToUtf8(const boost::filesystem::path& path);

	// Used for handling paths with non-ASCII characters.
	DLL_LINKAGE boost::filesystem::path Utf8TofilesystemPath(const std::string& path);

	/// Strip out unwanted characters from map name
	DLL_LINKAGE std::string convertMapName(std::string input);
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

template<typename Arithmetic>
inline Arithmetic TextOperations::parseMetric(const std::string &text)
{
	if (text.empty())
		return 0;

	// Trim whitespace
	std::string trimmed = text;
	trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch){ return !std::isspace(ch); }));
	trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), trimmed.end());

	// Check if last character is a metric suffix
	char last = trimmed.back();
	int power = 0; // number of *1000 multiplications

	switch (std::toupper(last))
	{
		case 'K': power = 1; break;
		case 'M': power = 2; break;
		case 'G': power = 3; break;
		case 'T': power = 4; break;
		case 'P': power = 5; break;
		case 'E': power = 6; break;
		default: power = 0; break; // no suffix
	}

	std::string numberPart = trimmed;
	if (power > 0)
		numberPart.pop_back();

	// Remove any non-digit or minus sign (same spirit as your numberFilter)
	numberPart.erase(std::remove_if(numberPart.begin(), numberPart.end(), [](char c)
	{
		return !(std::isdigit(static_cast<unsigned char>(c)) || c == '-');
	}), numberPart.end());

	if (numberPart.empty() || (numberPart == "-"))
		return 0;

	try
	{
		Arithmetic value = std::stoll(numberPart);

		for (int i = 0; i < power; ++i)
		{
			// Multiply by 1000, check for overflow if desired
			if (value > std::numeric_limits<Arithmetic>::max() / 1000)
				return std::numeric_limits<Arithmetic>::max();
			if (value < std::numeric_limits<Arithmetic>::min() / 1000)
				return std::numeric_limits<Arithmetic>::min();

			value *= static_cast<Arithmetic>(1000);
		}

		return value;
	}
	catch (std::invalid_argument &)
	{
		return 0;
	}
}

VCMI_LIB_NAMESPACE_END
