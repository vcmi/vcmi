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

size_t TextOperations::getUnicodeCharactersCount(const std::string & text)
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

int TextOperations::getLevenshteinDistance(std::string_view s, std::string_view t)
{
	int n = t.size();
	int m = s.size();

	// create two work vectors of integer distances
	std::vector<int> v0(n+1, 0);
	std::vector<int> v1(n+1, 0);

	// initialize v0 (the previous row of distances)
	// this row is A[0][i]: edit distance from an empty s to t;
	// that distance is the number of characters to append to  s to make t.
	for (int i = 0; i < n; ++i)
		v0[i] = i;

	for (int i = 0; i < m; ++i)
	{
		// calculate v1 (current row distances) from the previous row v0

		// first element of v1 is A[i + 1][0]
		// edit distance is delete (i + 1) chars from s to match empty t
		v1[0] = i + 1;

		// use formula to fill in the rest of the row
		for (int j = 0; j < n; ++j)
		{
			// calculating costs for A[i + 1][j + 1]
			int deletionCost = v0[j + 1] + 1;
			int insertionCost = v1[j] + 1;
			int substitutionCost;

			if (s[i] == t[j])
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

DLL_LINKAGE std::string TextOperations::getLocaleName()
{
	return Languages::getLanguageOptions(LIBRARY->generaltexth->getPreferredLanguage()).localeName;
}

DLL_LINKAGE bool TextOperations::compareLocalizedStrings(std::string_view str1, std::string_view str2)
{
	static const std::locale loc(getLocaleName());
	static const std::collate<char> & col = std::use_facet<std::collate<char>>(loc);

	return col.compare(str1.data(), str1.data() + str1.size(),
					   str2.data(), str2.data() + str2.size()) < 0;
}

std::optional<int> TextOperations::textSearchSimilarityScore(const std::string & s, const std::string & t)
{
	static const std::locale loc(getLocaleName());
	static const std::collate<char> & col = std::use_facet<std::collate<char>>(loc);

	auto haystack = col.transform(t.data(), t.data() + t.size());
	auto needle = col.transform(s.data(), s.data() + s.size());

	// 0 - Best possible match: text starts with the search string
	if(haystack.rfind(needle, 0) == 0)
		return 0;

	// 1 - Strong match: text contains the search string
	if(haystack.find(needle) != std::string::npos)
		return 1;

	// Dynamic threshold: Reject if too many typos based on word length
	int maxAllowedDistance = std::max(2, static_cast<int>(needle.size() / 2));

	// Compute Levenshtein distance for fuzzy similarity
	int minDist = std::numeric_limits<int>::max();
	for(size_t i = 0; i <= haystack.size() - needle.size(); i++)
	{
		int dist = getLevenshteinDistance(haystack.substr(i, needle.size()), needle);
		minDist = std::min(minDist, dist);
	}

	// Apply scaling: Short words tolerate smaller distances
	if(needle.size() > 2 && minDist <= 2)
		minDist += 1;

	return (minDist > maxAllowedDistance) ? std::nullopt : std::optional<int>{ minDist };
}

std::vector<size_t> TextOperations::getAllPositionsInStr(const char & targetChar, const std::string_view & str)
{
	std::vector<size_t> results;
	for(size_t pos = 0; pos < str.size(); pos++)
	{
		if(str[pos] == targetChar)
			results.push_back(pos);
	}
	return results;
};

std::optional<std::pair<size_t, size_t>> TextOperations::findFirstColorTag(const std::string_view & str)
{
	std::optional<std::pair<size_t, size_t>> res = std::nullopt;

	if(const auto colorTextBegin = str.find('{'); colorTextBegin != std::string::npos)
	{
		// Opened color tag brace found. Try to find color tag.
		if(const auto colorTagEnd = str.find('|'); colorTagEnd != std::string::npos)
			res = std::make_pair(colorTextBegin, colorTagEnd - colorTextBegin + 1);
		else
			res = std::make_pair(colorTextBegin, 1);
	}
	return res;
}

std::string_view TextOperations::findColorTagForSection(const std::string & str, size_t beginPos, size_t length)
{
	const auto segment = std::string_view(str).substr(beginPos, length);
	const auto tagBeginingPos = segment.find_last_of('{');
	const auto tagEndingPos = segment.find_last_of('}');

	if(tagBeginingPos != std::string::npos)
	{
		if(tagEndingPos == std::string::npos || tagEndingPos < tagBeginingPos)
		{
			const auto lastUnclosedColorTag = findFirstColorTag(segment.substr(tagBeginingPos, segment.length() - tagBeginingPos + 1));
			assert(lastUnclosedColorTag.has_value());
			return segment.substr(tagBeginingPos + lastUnclosedColorTag.value().first, lastUnclosedColorTag.value().second);
		}
	}
	else if(tagEndingPos == std::string::npos && beginPos > 0)
	{
		return findColorTagForSection(str, 0, beginPos);
	}
	return std::string_view();
}

size_t TextOperations::calcColorTagsSummaryWidth(const std::string & str, const std::function<size_t(const std::string&)> & getStringWidth)
{
	size_t width = 0;
	auto section = std::string_view(str).substr(0, str.length());

	while(const auto colorTag = findFirstColorTag(section))
	{
		width += getStringWidth(str.substr(colorTag.value().first, colorTag.value().second));
		section = section.substr(colorTag.value().first + colorTag.value().second);
	}
	width += std::count(str.begin(), str.end(), '}') * getStringWidth("}");
	return width;
}

bool TextOperations::validateColorBraces(const std::string & text)
{
	const auto openingBraces = getAllPositionsInStr('{', text);
	const auto closingBraces = getAllPositionsInStr('}', text);

	if(openingBraces.size() != closingBraces.size())
	{
		logGlobal->error("CMessage: Not Equal number opening and closing tags");
		return false;
	}

	if(openingBraces.empty())
		return true;

	auto closingPos = closingBraces.begin();
	size_t closingPrevPos = 0;
	for(const auto & openingPos : openingBraces)
	{
		if(openingPos >= *closingPos)
		{
			logGlobal->error("CMessage: Line validation Invalid: Closing tag before opening tag %s", text);
			return false;
		}
		if(openingPos < closingPrevPos)
		{
			logGlobal->error("CMessage: Line validation Invalid: Tags inside another tags %s", text);
			return false;
		}
		closingPrevPos = *closingPos++;
	}
	return true;
}

std::vector<TextOperations::TextSegment> TextOperations::getPossibleSublines(
	const std::string & line, size_t maxLineWidth, const std::function<size_t(const std::string&)> & getStringWidth, const char & splitSymbol)
{
	std::vector<TextSegment> result;
	std::string_view lastFoundColor;

	const std::function<void(const std::string_view&)> findPossibleSplit = [&](const std::string_view & subLine)
	{
		// Trim splitSymbols from beginning and ending
		const auto subLineBeginPos = subLine.find_first_not_of(splitSymbol);
		if(subLineBeginPos == std::string::npos)
			return;
		const auto subLineEndPos = subLine.find_last_not_of(splitSymbol);
		if(subLineEndPos == std::string::npos)
			return;

		const auto trimmedLine = subLine.substr(subLineBeginPos, subLineEndPos - subLineBeginPos + 1);
		size_t wordBegin = 0;
		const auto splitPossibles = getAllPositionsInStr(splitSymbol, trimmedLine);
		if(splitPossibles.empty())
		{
			result.back().line = std::make_pair(trimmedLine.data() - line.data(), trimmedLine.length());
			return;
		}

		size_t lineWidth = 0;
		auto word = trimmedLine.substr(wordBegin, splitPossibles.front());
		size_t wordWidth = getStringWidth(std::string(word)) - calcColorTagsSummaryWidth(std::string(word), getStringWidth);
		lastFoundColor = findColorTagForSection(line, word.data() - line.data(), word.length());

		for(auto splitPos = splitPossibles.cbegin(); splitPos < splitPossibles.cend(); splitPos = std::next(splitPos))
		{
			lineWidth += wordWidth;
			const auto curColor = lastFoundColor;
			const auto wordEnd = *splitPos;
			const auto nextSplitPos = std::next(splitPos);
			word = nextSplitPos == splitPossibles.end() ? trimmedLine.substr(wordEnd, trimmedLine.length() - wordEnd)
				: trimmedLine.substr(wordEnd, *nextSplitPos - wordEnd);
			wordWidth = getStringWidth(std::string(word)) - calcColorTagsSummaryWidth(std::string(word), getStringWidth);
			lastFoundColor = findColorTagForSection(line, word.data() - line.data(), word.length());

			if(lineWidth + wordWidth > maxLineWidth)
			{
				const auto resultLine = trimmedLine.substr(0, wordEnd);
				// Trim splitSymbols from ending and place result line into result
				result.back().line = std::make_pair(resultLine.data() - line.data(), resultLine.find_last_not_of(splitSymbol) + 1);
				if(curColor.empty())
				{
					result.emplace_back();
				}
				else
				{
					result.back().closingTagNeeded = true;
					result.emplace_back(std::optional<std::pair<size_t, size_t>>({ curColor.data() - line.data(), curColor.length() }));
				}
				findPossibleSplit(trimmedLine.substr(wordEnd, trimmedLine.length() - wordEnd));
				return;
			}
			else
			{
				if(wordBegin == wordEnd)
					lineWidth += getStringWidth(std::string(1, splitSymbol));
				wordBegin = wordEnd + 1;
			}
		}
		result.back().line = std::make_pair(trimmedLine.data() - line.data(), trimmedLine.length());
	};

	result.emplace_back();
	findPossibleSplit(line);

	return result;
}

std::vector<std::string> TextOperations::breakText(
	const std::string & text,
	size_t maxLineWidth,
	const std::function<size_t(const std::string&)> & getStringWidth)
{
	std::vector<std::string> result;
	if(!validateColorBraces(text))
		return result;

	// Detect end of line symbol. If '\r\n', remove '\r'.
	// Texts with '\r\n' are very rare (Credits case), so rare string copying should not be a problem.
	std::string preparedText;
	auto preparedTextPtr = &text;
	auto breakSymbol = '\n';
	if(text.find("\r\n") != std::string::npos)
	{
		preparedText = text;
		preparedTextPtr = &preparedText;
		preparedText.erase(std::remove(preparedText.begin(), preparedText.end(), '\r'), preparedText.end());
	}

	// Split text by lines
	auto possibleLines = getPossibleSublines(*preparedTextPtr, 0, getStringWidth, breakSymbol);

	// Add additional empty lines if needed. In Case "{....\n\n....}"
	const auto possibleLinesOrigin = possibleLines;
	auto curLine = possibleLinesOrigin.cbegin();
	auto insertionIterator = possibleLines.cbegin();
	size_t nextLineBeginEstimate = 0;
	auto newLines = curLine->line.first - nextLineBeginEstimate;

	for(auto nextLine = std::next(curLine);; nextLine = std::next(nextLine))
	{
		for(size_t newBreak = 0; newBreak < newLines; newBreak++)
		{
			const auto newLine = possibleLines.emplace(insertionIterator);
			newLine->line.first = nextLineBeginEstimate + newBreak;
			insertionIterator = std::next(newLine);
		}
		if(nextLine >= possibleLinesOrigin.cend())
			break;

		nextLineBeginEstimate = curLine->line.first + curLine->line.second + 1;
		newLines = nextLine->line.first - nextLineBeginEstimate;
		assert(nextLineBeginEstimate <= nextLine->line.first);
		curLine = nextLine;
		insertionIterator = std::next(insertionIterator);
	}

	const auto makeNewLine = [](const auto & lineBreak, const auto & srcStr, auto & emptyLine)
	{
		const auto colorTag = lineBreak.colorTag ?
			std::string_view(srcStr).substr(lineBreak.colorTag.value().first, lineBreak.colorTag.value().second) : std::string_view();
		emptyLine = std::string(colorTag) + srcStr.substr(lineBreak.line.first, lineBreak.line.second);
		if(lineBreak.closingTagNeeded)
			emptyLine.append("}");
	};

	// This is necessary to avoid spaces at the beginning of the line, after the color tag. "{  ...."
	for(auto & lineBreak : possibleLines)
		if(lineBreak.colorTag)
		{
			const auto lineBeginPos = preparedTextPtr->find_first_not_of(' ', lineBreak.line.first);
			assert(lineBeginPos < lineBreak.line.first + lineBreak.line.second);
			lineBreak.line.second -= lineBeginPos - lineBreak.line.first;
			lineBreak.line.first = lineBeginPos;
		}

	// Split lines by spaces and fill result
	for(const auto & lineBreak : possibleLines)
	{
		std::string line;
		makeNewLine(lineBreak, *preparedTextPtr, line);
		for(const auto & breakBySpace : getPossibleSublines(line, maxLineWidth, getStringWidth, ' '))
			makeNewLine(breakBySpace, line, result.emplace_back());
	}
	return result;
}

VCMI_LIB_NAMESPACE_END
