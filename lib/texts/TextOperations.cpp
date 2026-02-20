/*
 * TextOperations.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TextOperations.h"

#include "../GameLibrary.h"
#include "../texts/CGeneralTextHandler.h"
#include "Languages.h"
#include "CConfigHandler.h"

#include <vstd/DateUtils.h>

#include <boost/locale/encoding.hpp>

VCMI_LIB_NAMESPACE_BEGIN

size_t TextOperations::getUnicodeCharacterSize(char firstByte)
{
	// length of utf-8 character can be determined from 1st byte by counting number of highest bits set to 1:
	// 0xxxxxxx -> 1 -  ASCII chars
	// 110xxxxx -> 2
	// 1110xxxx -> 3
	// 11110xxx -> 4 - last allowed in current standard

	auto value = static_cast<uint8_t>(firstByte);

	if ((value & 0b10000000) == 0)
		return 1; // ASCII

	if ((value & 0b11100000) == 0b11000000)
		return 2;

	if ((value & 0b11110000) == 0b11100000)
		return 3;

	if ((value & 0b11111000) == 0b11110000)
		return 4;

	assert(0);// invalid unicode sequence
	return 4;
}

bool TextOperations::isValidUnicodeCharacter(const char * character, size_t maxSize)
{
	assert(maxSize > 0);

	auto value = static_cast<uint8_t>(character[0]);

	// ASCII
	if ( value < 0b10000000)
		return maxSize > 0;

	// can't be first byte in UTF8
	if (value < 0b11000000)
		return false;

	// above maximum allowed in standard (UTF codepoints are capped at 0x0010FFFF)
	if (value > 0b11110000)
		return false;

	// first character must follow rules checked in getUnicodeCharacterSize
	size_t size = getUnicodeCharacterSize(character[0]);

	if (size > maxSize)
		return false;

	// remaining characters must have highest bit set to 1
	for (size_t i = 1; i < size; i++)
	{
		auto characterValue = static_cast<uint8_t>(character[i]);
		if (characterValue < 0b10000000)
			return false;
	}
	return true;
}

bool TextOperations::isValidASCII(const std::string & text)
{
	for (const char & ch : text)
		if (static_cast<uint8_t>(ch) >= 0x80 )
			return false;
	return true;
}

bool TextOperations::isValidASCII(const char * data, size_t size)
{
	for (size_t i=0; i<size; i++)
		if (static_cast<uint8_t>(data[i]) >= 0x80 )
			return false;
	return true;
}

bool TextOperations::isValidUnicodeString(std::string_view text)
{
	for (size_t i=0; i<text.size(); i += getUnicodeCharacterSize(text[i]))
	{
		if (!isValidUnicodeCharacter(text.data() + i, text.size() - i))
			return false;
	}
	return true;
}

bool TextOperations::isValidUnicodeString(const char * data, size_t size)
{
	for (size_t i=0; i<size; i += getUnicodeCharacterSize(data[i]))
	{
		if (!isValidUnicodeCharacter(data + i, size - i))
			return false;
	}
	return true;
}

uint32_t TextOperations::getUnicodeCodepoint(const char * data, size_t maxSize)
{
	assert(isValidUnicodeCharacter(data, maxSize));
	if (!isValidUnicodeCharacter(data, maxSize))
		return 0;

	// https://en.wikipedia.org/wiki/UTF-8#Encoding
	switch (getUnicodeCharacterSize(data[0]))
	{
		case 1:
			return static_cast<uint8_t>(data[0]) & 0b1111111;
		case 2:
			return
				((static_cast<uint8_t>(data[0]) & 0b11111 ) << 6) +
				((static_cast<uint8_t>(data[1]) & 0b111111) << 0) ;
		case 3:
			return
				((static_cast<uint8_t>(data[0]) & 0b1111 )  << 12) +
				((static_cast<uint8_t>(data[1]) & 0b111111) << 6) +
				((static_cast<uint8_t>(data[2]) & 0b111111) << 0) ;
		case 4:
			return
				((static_cast<uint8_t>(data[0]) & 0b111 )   << 18) +
				((static_cast<uint8_t>(data[1]) & 0b111111) << 12) +
				((static_cast<uint8_t>(data[2]) & 0b111111) << 6) +
				((static_cast<uint8_t>(data[3]) & 0b111111) << 0) ;
	}

	assert(0);
	return 0;
}

uint32_t TextOperations::getUnicodeCodepoint(char data, const std::string & encoding )
{
	std::string stringNative(1, data);
	std::string stringUnicode = toUnicode(stringNative, encoding);

	if (stringUnicode.empty())
		return 0;

	return getUnicodeCodepoint(stringUnicode.data(), stringUnicode.size());
}

std::string TextOperations::toUnicode(const std::string &text, const std::string &encoding)
{
	try {
		return boost::locale::conv::to_utf<char>(text, encoding);
	}
	catch (const boost::locale::conv::conversion_error &)
	{
		throw std::runtime_error("Failed to convert text '" + text + "' from encoding " + encoding );
	}
}

std::string TextOperations::fromUnicode(const std::string &text, const std::string &encoding)
{
	try {
		return boost::locale::conv::from_utf<char>(text, encoding);
	}
	catch (const boost::locale::conv::conversion_error &)
	{
		throw std::runtime_error("Failed to convert text '" + text + "' to encoding " + encoding );
	}
}

void TextOperations::trimRightUnicode(std::string & text, const size_t amount)
{
	if(text.empty())
		return;
	//todo: more efficient algorithm
	for(int i = 0; i< amount; i++){
		auto b = text.begin();
		auto e = text.end();
		size_t lastLen = 0;
		size_t len = 0;
		while (b != e) {
			lastLen = len;
			size_t n = getUnicodeCharacterSize(*b);

			if(!isValidUnicodeCharacter(&(*b),e-b))
			{
				logGlobal->error("Invalid UTF8 sequence");
				break;//invalid sequence will be trimmed
			}

			len += n;
			b += n;
		}

		text.resize(lastLen);
	}
}

size_t TextOperations::getUnicodeCharactersCount(std::string_view text)
{
	size_t charactersCount = 0;

	for (size_t i=0; i<text.size(); i += getUnicodeCharacterSize(text[i]))
		charactersCount++;

	return charactersCount;
}

std::string TextOperations::escapeString(std::string input)
{
	boost::replace_all(input, "\\", "\\\\");
	boost::replace_all(input, "\n", "\\n");
	boost::replace_all(input, "\r", "\\r");
	boost::replace_all(input, "\t", "\\t");
	boost::replace_all(input, "\"", "\\\"");

	return input;
}

std::string TextOperations::getFormattedDateTimeLocal(std::time_t dt)
{
	return vstd::getFormattedDateTime(dt, Languages::getLanguageOptions(settings["general"]["language"].String()).dateTimeFormat);
}

std::string TextOperations::getFormattedTimeLocal(std::time_t dt)
{
	return vstd::getFormattedDateTime(dt, "%H:%M");
}

std::string TextOperations::getCurrentFormattedTimeLocal(std::chrono::seconds timeOffset)
{
	auto timepoint = std::chrono::system_clock::now() + timeOffset;
	return TextOperations::getFormattedTimeLocal(std::chrono::system_clock::to_time_t(timepoint));
}

std::string TextOperations::getCurrentFormattedDateTimeLocal(std::chrono::seconds timeOffset)
{
	auto timepoint = std::chrono::system_clock::now() + timeOffset;
	return TextOperations::getFormattedDateTimeLocal(std::chrono::system_clock::to_time_t(timepoint));
}

static const std::locale & getLocale()
{
	auto getLocale = []() -> std::locale
	{
		const std::string & baseLocaleName = Languages::getLanguageOptions(LIBRARY->generaltexth->getPreferredLanguage()).localeName;
		const std::string fallbackLocale = Languages::getLanguageOptions(Languages::ELanguages::ENGLISH).localeName;

		for (const auto & localeName : { baseLocaleName + ".UTF-8", baseLocaleName, fallbackLocale + ".UTF-8", fallbackLocale })
		{
			try
			{
				// Locale generation may fail (and throw an exception) in two cases:
				// - if the corresponding locale is not installed on the system
				// - on Android named locales are not supported at all and always throw an exception
				return std::locale(localeName);
			}
			catch (const std::exception & e)
			{
				logGlobal->warn("Failed to set locale '%s'", localeName);
			}
		}
		return std::locale();
	};

	static const std::locale locale = getLocale();
	return locale;
}

int TextOperations::getLevenshteinDistance(std::string_view s, std::string_view t)
{
	assert(isValidUnicodeString(s));
	assert(isValidUnicodeString(t));

	auto charactersEqual = [&s, &t](int sPoint, int tPoint)
	{
		uint32_t sUTF32 = getUnicodeCodepoint(s.data() + sPoint, s.size() - sPoint);
		uint32_t tUTF32 = getUnicodeCodepoint(t.data() + tPoint, t.size() - tPoint);

		if (sUTF32 == tUTF32)
			return true;

		// Windows - wchar_t represents UTF-16 symbol that does not cover entire Unicode
		// In UTF-16 such characters can only be represented as 2 wchar_t's, but toupper can only operate on single wchar
		// Assume symbols are different if one of them cannot be represented as a single UTF-16 symbol
		if constexpr (sizeof(wchar_t) == 2)
		{
			if (sUTF32 > 0xFFFF || (sUTF32 >= 0xD800 && sUTF32 <= 0xDFFF ))
				return false;

			if (tUTF32 > 0xFFFF || (tUTF32 >= 0xD800 && tUTF32 <= 0xDFFF ))
				return false;
		}

		const auto & facet = std::use_facet<std::ctype<wchar_t>>(getLocale());
		return facet.toupper(sUTF32) == facet.toupper(tUTF32);
	};

	int n = getUnicodeCharactersCount(t);
	int m = getUnicodeCharactersCount(s);

	// create two work vectors of integer distances
	std::vector<int> v0(n+1, 0);
	std::vector<int> v1(n+1, 0);

	// initialize v0 (the previous row of distances)
	// this row is A[0][i]: edit distance from an empty s to t;
	// that distance is the number of characters to append to  s to make t.
	for (int i = 0; i < n; ++i)
		v0[i] = i;

	for (int i = 0, iPoint = 0; i < m; ++i, iPoint += getUnicodeCharacterSize(s[iPoint]))
	{

		// calculate v1 (current row distances) from the previous row v0

		// first element of v1 is A[i + 1][0]
		// edit distance is delete (i + 1) chars from s to match empty t
		v1[0] = i + 1;

		// use formula to fill in the rest of the row
		for (int j = 0, jPoint = 0; j < n; ++j, jPoint += getUnicodeCharacterSize(t[jPoint]))
		{
			// calculating costs for A[i + 1][j + 1]
			int deletionCost = v0[j + 1] + 1;
			int insertionCost = v1[j] + 1;
			int substitutionCost;

			if (charactersEqual(iPoint, jPoint))
				substitutionCost = v0[j];
			else
				substitutionCost = v0[j] + 1;

			v1[j + 1] = std::min({deletionCost, insertionCost, substitutionCost});
		}

		// copy v1 (current row) to v0 (previous row) for next iteration
		// since data in v1 is always invalidated, a swap without copy could be more efficient
		std::swap(v0, v1);
	}

	// after the last swap, the results of v1 are now in v0
	return v0[n];
}

DLL_LINKAGE bool TextOperations::compareLocalizedStrings(std::string_view str1, std::string_view str2)
{
	const std::collate<char> & col = std::use_facet<std::collate<char>>(getLocale());
	return col.compare(
		str1.data(), str1.data() + str1.size(),
		str2.data(), str2.data() + str2.size()
	) < 0;
}

std::optional<int> TextOperations::textSearchSimilarityScore(const std::string & needle, const std::string & haystack)
{
	// 0 - Best possible match: text starts with the search string
	if(haystack.rfind(needle, 0) == 0)
		return 0;

	// 1 - Strong match: text contains the search string
	if(haystack.find(needle) != std::string::npos)
		return 1;

	// Dynamic threshold: Reject if too many typos based on word length
	int haystackCodepoints = getUnicodeCharactersCount(haystack);
	int needleCodepoints = getUnicodeCharactersCount(needle);
	int maxAllowedDistance = needleCodepoints / 2;

	// Compute Levenshtein distance for fuzzy similarity
	int minDist = std::numeric_limits<int>::max();

	for(int i = 0; i <= haystackCodepoints - needleCodepoints; ++i)
	{
		int haystackBegin = 0;
		for(int j = 0; j < i; ++j)
			haystackBegin += getUnicodeCharacterSize(haystack[haystackBegin]);

		int haystackEnd = haystackBegin;
		for(int j = 0; j < needleCodepoints; ++j)
			haystackEnd += getUnicodeCharacterSize(haystack[haystackEnd]);

		int dist = getLevenshteinDistance(haystack.substr(haystackBegin, haystackEnd - haystackBegin), needle);
		minDist = std::min(minDist, dist);
	}

	// Apply scaling: Short words tolerate smaller distances
	if(needle.size() > 2 && minDist <= 2)
		minDist += 1;

	return (minDist > maxAllowedDistance) ? std::nullopt : std::optional<int>{ minDist };
}

std::string TextOperations::filesystemPathToUtf8(const boost::filesystem::path& path)
{
#ifdef VCMI_WINDOWS
	return boost::locale::conv::utf_to_utf<char>(path.native());
#else
	return path.string();
#endif
}

boost::filesystem::path TextOperations::Utf8TofilesystemPath(const std::string& path)
{
#ifdef VCMI_WINDOWS
	return boost::filesystem::path(boost::locale::conv::utf_to_utf<wchar_t>(path));
#else
	return boost::filesystem::path(path);
#endif
}

std::string TextOperations::convertMapName(std::string input)
{
	boost::algorithm::to_lower(input);
	boost::algorithm::trim(input);
	boost::algorithm::erase_all(input, ".");

	size_t slashPos = input.find_last_of('/');

	if(slashPos != std::string::npos)
		return input.substr(slashPos + 1);

	return input;
}

VCMI_LIB_NAMESPACE_END
