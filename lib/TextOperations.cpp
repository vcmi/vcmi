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

size_t Unicode::getCharacterSize(char firstByte)
{
	// length of utf-8 character can be determined from 1st byte by counting number of highest bits set to 1:
	// 0xxxxxxx -> 1 -  ASCII chars
	// 110xxxxx -> 2
	// 11110xxx -> 4 - last allowed in current standard
	// 1111110x -> 6 - last allowed in original standard

	if ((ui8)firstByte < 0x80)
		return 1; // ASCII

	size_t ret = 0;

	for (size_t i=0; i<8; i++)
	{
		if (((ui8)firstByte & (0x80 >> i)) != 0)
			ret++;
		else
			break;
	}
	return ret;
}

bool Unicode::isValidCharacter(const char * character, size_t maxSize)
{
	// can't be first byte in UTF8
	if ((ui8)character[0] >= 0x80 && (ui8)character[0] < 0xC0)
		return false;
	// first character must follow rules checked in getCharacterSize
	size_t size = getCharacterSize((ui8)character[0]);

	if ((ui8)character[0] > 0xF4)
		return false; // above maximum allowed in standard (UTF codepoints are capped at 0x0010FFFF)

	if (size > maxSize)
		return false;

	// remaining characters must have highest bit set to 1
	for (size_t i = 1; i < size; i++)
	{
		if (((ui8)character[i] & 0x80) == 0)
			return false;
	}
	return true;
}

bool Unicode::isValidASCII(const std::string & text)
{
	for (const char & ch : text)
		if (ui8(ch) >= 0x80 )
			return false;
	return true;
}

bool Unicode::isValidASCII(const char * data, size_t size)
{
	for (size_t i=0; i<size; i++)
		if (ui8(data[i]) >= 0x80 )
			return false;
	return true;
}

bool Unicode::isValidString(const std::string & text)
{
	for (size_t i=0; i<text.size(); i += getCharacterSize(text[i]))
	{
		if (!isValidCharacter(text.data() + i, text.size() - i))
			return false;
	}
	return true;
}

bool Unicode::isValidString(const char * data, size_t size)
{
	for (size_t i=0; i<size; i += getCharacterSize(data[i]))
	{
		if (!isValidCharacter(data + i, size - i))
			return false;
	}
	return true;
}

std::string Unicode::toUnicode(const std::string &text)
{
	return toUnicode(text, CGeneralTextHandler::getInstalledEncoding());
}

std::string Unicode::toUnicode(const std::string &text, const std::string &encoding)
{
	return boost::locale::conv::to_utf<char>(text, encoding);
}

std::string Unicode::fromUnicode(const std::string & text)
{
	return fromUnicode(text, CGeneralTextHandler::getInstalledEncoding());
}

std::string Unicode::fromUnicode(const std::string &text, const std::string &encoding)
{
	return boost::locale::conv::from_utf<char>(text, encoding);
}

void Unicode::trimRight(std::string & text, const size_t amount)
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
			size_t n = getCharacterSize(*b);

			if(!isValidCharacter(&(*b),e-b))
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

VCMI_LIB_NAMESPACE_END
