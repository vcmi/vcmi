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

#include "CGeneralTextHandler.h"

#include <boost/locale.hpp>

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

bool TextOperations::isValidUnicodeString(const std::string & text)
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
	return boost::locale::conv::to_utf<char>(text, encoding);
}

std::string TextOperations::fromUnicode(const std::string &text, const std::string &encoding)
{
	return boost::locale::conv::from_utf<char>(text, encoding);
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

size_t getUnicodeCharactersCount(const std::string & text)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	return conv.from_bytes(text).size(); 
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

VCMI_LIB_NAMESPACE_END
