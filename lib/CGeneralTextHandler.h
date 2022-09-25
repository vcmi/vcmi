/*
 * CGeneralTextHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "JsonNode.h"

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
	DLL_LINKAGE void trimRight(std::string & text, const size_t amount = 1);
};

class CInputStream;

/// Parser for any text files from H3
class DLL_LINKAGE CLegacyConfigParser
{
	std::unique_ptr<char[]> data;
	char * curr;
	char * end;

	void init(const std::unique_ptr<CInputStream> & input);

	/// extracts part of quoted string.
	std::string extractQuotedPart();

	/// extracts quoted string. Any end of lines are ignored, double-quote is considered as "escaping"
	std::string extractQuotedString();

	/// extracts non-quoted string
	std::string extractNormalString();

	/// reads "raw" string without encoding conversion
	std::string readRawString();
public:
	/// read one entry from current line. Return ""/0 if end of line reached
	std::string readString();
	float readNumber();

	template <typename numeric>
	std::vector<numeric> readNumArray(size_t size)
	{
		std::vector<numeric> ret;
		ret.reserve(size);
		while (size--)
			ret.push_back((numeric)readNumber());
		return ret;
	}

	/// returns true if next entry is empty
	bool isNextEntryEmpty() const;

	/// end current line
	bool endLine();

	CLegacyConfigParser(std::string URI);
	CLegacyConfigParser(const std::unique_ptr<CInputStream> & input);
};

class DLL_LINKAGE CGeneralTextHandler //Handles general texts
{
public:
	JsonNode localizedTexts;

	std::vector<std::string> allTexts;

	std::vector<std::string> arraytxt;
	std::vector<std::string> primarySkillNames;
	std::vector<std::string> jktexts;
	std::vector<std::string> heroscrn;
	std::vector<std::string> overview;//text for Kingdom Overview window
	std::vector<std::string> colors; //names of player colors ("red",...)
	std::vector<std::string> capColors; //names of player colors with first letter capitalized ("Red",...)
	std::vector<std::string> turnDurations; //turn durations for pregame (1 Minute ... Unlimited)

	//towns
	std::vector<std::string> tcommands, hcommands, fcommands; //texts for town screen, town hall screen and fort screen
	std::vector<std::string> tavernInfo;
	std::vector<std::string> tavernRumors;

	std::vector<std::string> qeModCommands;

	std::vector<std::pair<std::string,std::string>> zelp;
	std::vector<std::string> lossCondtions;
	std::vector<std::string> victoryConditions;

	//objects
	std::vector<std::string> creGens; //names of creatures' generators
	std::vector<std::string> creGens4; //names of multiple creatures' generators
	std::vector<std::string> advobtxt;
	std::vector<std::string> xtrainfo;
	std::vector<std::string> restypes; //names of resources
	std::map<TTerrainId, std::string> terrainNames;
	std::vector<std::string> randsign;
	std::vector<std::pair<std::string,std::string>> mines; //first - name; second - event description
	std::vector<std::string> seerEmpty;
	std::vector<std::vector<std::vector<std::string>>>  quests; //[quest][type][index]
	//type: quest, progress, complete, rollover, log OR time limit //index: 0-2 seer hut, 3-5 border guard
	std::vector<std::string> seerNames;
	std::vector<std::string> tentColors;

	//sec skills
	std::vector<std::string> levels;
	std::vector<std::string> zcrexp; //more or less useful content of that file
	//commanders
	std::vector<std::string> znpc00; //more or less useful content of that file

	//campaigns
	std::vector<std::string> campaignMapNames;
	std::vector<std::vector<std::string>> campaignRegionNames;

	static void readToVector(std::string sourceName, std::vector<std::string> &dest);

	int32_t pluralText(const int32_t textIndex, const int32_t count) const;

	CGeneralTextHandler();
	CGeneralTextHandler(const CGeneralTextHandler&) = delete;
	CGeneralTextHandler operator=(const CGeneralTextHandler&) = delete;
};
