/*
 * CGeneralTextHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGeneralTextHandler.h"

#include <boost/locale.hpp>

#include "filesystem/Filesystem.h"
#include "CConfigHandler.h"
#include "CModHandler.h"
#include "GameConstants.h"
#include "VCMI_Lib.h"
#include "Terrain.h"

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

static std::string getSelectedEncoding()
{
	return settings["general"]["encoding"].String();
}

std::string Unicode::toUnicode(const std::string &text)
{
	return toUnicode(text, getSelectedEncoding());
}

std::string Unicode::toUnicode(const std::string &text, const std::string &encoding)
{
	return boost::locale::conv::to_utf<char>(text, encoding);
}

std::string Unicode::fromUnicode(const std::string & text)
{
	return fromUnicode(text, getSelectedEncoding());
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


//Helper for string -> float conversion
class LocaleWithComma: public std::numpunct<char>
{
protected:
	char do_decimal_point() const override
	{
		return ',';
	}
};

CLegacyConfigParser::CLegacyConfigParser(std::string URI)
{
	init(CResourceHandler::get()->load(ResourceID(URI, EResType::TEXT)));
}

CLegacyConfigParser::CLegacyConfigParser(const std::unique_ptr<CInputStream> & input)
{
	init(input);
}

void CLegacyConfigParser::init(const std::unique_ptr<CInputStream> & input)
{
	data.reset(new char[input->getSize()]);
	input->read((ui8*)data.get(), input->getSize());

	curr = data.get();
	end = curr + input->getSize();
}

std::string CLegacyConfigParser::extractQuotedPart()
{
	assert(*curr == '\"');

	curr++; // skip quote
	char * begin = curr;

	while (curr != end && *curr != '\"' && *curr != '\t')
		curr++;

	return std::string(begin, curr++); //increment curr to close quote
}

std::string CLegacyConfigParser::extractQuotedString()
{
	assert(*curr == '\"');

	std::string ret;
	while (true)
	{
		ret += extractQuotedPart();

		// double quote - add it to string and continue quoted part
		if (curr < end && *curr == '\"')
		{
			ret += '\"';
		}
		//extract normal part
		else if(curr < end && *curr != '\t' && *curr != '\r')
		{
			char * begin = curr;

			while (curr < end && *curr != '\t' && *curr != '\r' && *curr != '\"')//find end of string or next quoted part start
				curr++;

			ret += std::string(begin, curr);

			if(curr>=end || *curr != '\"')
				return ret;
		}
		else // end of string
			return ret;
	}
}

std::string CLegacyConfigParser::extractNormalString()
{
	char * begin = curr;

	while (curr < end && *curr != '\t' && *curr != '\r')//find end of string
		curr++;

	return std::string(begin, curr);
}

std::string CLegacyConfigParser::readRawString()
{
	if (curr >= end || *curr == '\n')
		return "";

	std::string ret;

	if (*curr == '\"')
		ret = extractQuotedString();// quoted text - find closing quote
	else
		ret = extractNormalString();//string without quotes - copy till \t or \r

	curr++;
	return ret;
}

std::string CLegacyConfigParser::readString()
{
	// do not convert strings that are already in ASCII - this will only slow down loading process
	std::string str = readRawString();
	if (Unicode::isValidASCII(str))
		return str;
	return Unicode::toUnicode(str);
}

float CLegacyConfigParser::readNumber()
{
	std::string input = readRawString();

	std::istringstream stream(input);

	if(input.find(',') != std::string::npos) // code to handle conversion with comma as decimal separator
		stream.imbue(std::locale(std::locale(), new LocaleWithComma()));

	float result;
	if ( !(stream >> result) )
		return 0;
	return result;
}

bool CLegacyConfigParser::isNextEntryEmpty() const
{
	char * nextSymbol = curr;
	while (nextSymbol < end && *nextSymbol == ' ')
		nextSymbol++; //find next meaningfull symbol

	return nextSymbol >= end || *nextSymbol == '\n' || *nextSymbol == '\r' || *nextSymbol == '\t';
}

bool CLegacyConfigParser::endLine()
{
	while (curr < end && *curr !=  '\n')
		readString();

	curr++;

	return curr < end;
}

void CGeneralTextHandler::readToVector(std::string sourceName, std::vector<std::string> &dest)
{
	CLegacyConfigParser parser(sourceName);
	do
	{
		dest.push_back(parser.readString());
	}
	while (parser.endLine());
}

CGeneralTextHandler::CGeneralTextHandler()
{
	std::vector<std::string> h3mTerrainNames;
	readToVector("DATA/VCDESC.TXT",   victoryConditions);
	readToVector("DATA/LCDESC.TXT",   lossCondtions);
	readToVector("DATA/TCOMMAND.TXT", tcommands);
	readToVector("DATA/HALLINFO.TXT", hcommands);
	readToVector("DATA/CASTINFO.TXT", fcommands);
	readToVector("DATA/ADVEVENT.TXT", advobtxt);
	readToVector("DATA/XTRAINFO.TXT", xtrainfo);
	readToVector("DATA/RESTYPES.TXT", restypes);
	readToVector("DATA/TERRNAME.TXT", h3mTerrainNames);
	readToVector("DATA/RANDSIGN.TXT", randsign);
	readToVector("DATA/CRGEN1.TXT",   creGens);
	readToVector("DATA/CRGEN4.TXT",   creGens4);
	readToVector("DATA/OVERVIEW.TXT", overview);
	readToVector("DATA/ARRAYTXT.TXT", arraytxt);
	readToVector("DATA/PRISKILL.TXT", primarySkillNames);
	readToVector("DATA/JKTEXT.TXT",   jktexts);
	readToVector("DATA/TVRNINFO.TXT", tavernInfo);
	readToVector("DATA/RANDTVRN.TXT", tavernRumors);
	readToVector("DATA/TURNDUR.TXT",  turnDurations);
	readToVector("DATA/HEROSCRN.TXT", heroscrn);
	readToVector("DATA/TENTCOLR.TXT", tentColors);
	readToVector("DATA/SKILLLEV.TXT", levels);
	
	for(int i = 0; i < h3mTerrainNames.size(); ++i)
	{
		terrainNames[Terrain::createTerrainTypeH3M(i)] = h3mTerrainNames[i];
	}
	for(auto & terrain : Terrain::Manager::terrains())
	{
		if(!Terrain::Manager::getInfo(terrain).terrainText.empty())
			terrainNames[terrain] = Terrain::Manager::getInfo(terrain).terrainText;
	}
	

	static const char * QE_MOD_COMMANDS = "DATA/QECOMMANDS.TXT";
	if (CResourceHandler::get()->existsResource(ResourceID(QE_MOD_COMMANDS, EResType::TEXT)))
		readToVector(QE_MOD_COMMANDS, qeModCommands);

	localizedTexts = JsonNode(ResourceID("config/translate.json", EResType::TEXT));

	{
		CLegacyConfigParser parser("DATA/GENRLTXT.TXT");
		parser.endLine();
		do
		{
			allTexts.push_back(parser.readString());
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/HELP.TXT");
		do
		{
			std::string first = parser.readString();
			std::string second = parser.readString();
			zelp.push_back(std::make_pair(first, second));
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser nameParser("DATA/MINENAME.TXT");
		CLegacyConfigParser eventParser("DATA/MINEEVNT.TXT");

		do
		{
			std::string name  = nameParser.readString();
			std::string event = eventParser.readString();
			mines.push_back(std::make_pair(name, event));
		}
		while (nameParser.endLine() && eventParser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/PLCOLORS.TXT");
		do
		{
			std::string color = parser.readString();
			colors.push_back(color);

			color[0] = toupper(color[0]);
			capColors.push_back(color);
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/SEERHUT.TXT");

		//skip header
		parser.endLine();

		for (int i = 0; i < 6; ++i)
			seerEmpty.push_back(parser.readString());
		parser.endLine();

		quests.resize(10);
		for (int i = 0; i < 9; ++i) //9 types of quests
		{
			quests[i].resize(5);
			for (int j = 0; j < 5; ++j)
			{
				parser.readString(); //front description
				for (int k = 0; k < 6; ++k)
					quests[i][j].push_back(parser.readString());

				parser.endLine();
			}
		}
		quests[9].resize(1);

		for (int k = 0; k < 6; ++k) //Time limit
		{
			quests[9][0].push_back(parser.readString());
		}
		parser.endLine();

		parser.endLine(); // empty line
		parser.endLine(); // header

		for (int i = 0; i < 48; ++i)
		{
			seerNames.push_back(parser.readString());
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/CAMPTEXT.TXT");

		//skip header
		parser.endLine();

		std::string text;
		do
		{
			text = parser.readString();
			if (!text.empty())
				campaignMapNames.push_back(text);
		}
		while (parser.endLine() && !text.empty());

		for (size_t i=0; i<campaignMapNames.size(); i++)
		{
			do // skip empty space and header
			{
				text = parser.readString();
			}
			while (parser.endLine() && text.empty());

			campaignRegionNames.push_back(std::vector<std::string>());
			do
			{
				text = parser.readString();
				if (!text.empty())
					campaignRegionNames.back().push_back(text);
			}
			while (parser.endLine() && !text.empty());
		}
	}
	if (VLC->modh->modules.STACK_EXP)
	{
		CLegacyConfigParser parser("DATA/ZCREXP.TXT");
		parser.endLine();//header
		for (size_t iter=0; iter<325; iter++)
		{
			parser.readString(); //ignore 1st column with description
			zcrexp.push_back(parser.readString());
			parser.endLine();
		}
		// line 325 - some weird formatting here
		zcrexp.push_back(parser.readString());
		parser.readString();
		parser.endLine();

		do // rest of file can be read normally
		{
			parser.readString(); //ignore 1st column with description
			zcrexp.push_back(parser.readString());
		}
		while (parser.endLine());
	}
	if (VLC->modh->modules.COMMANDERS)
	{
		try
		{
			CLegacyConfigParser parser("DATA/ZNPC00.TXT");
			parser.endLine();//header

			do
			{
				znpc00.push_back(parser.readString());
			} while (parser.endLine());
		}
		catch (const std::runtime_error &)
		{
			logGlobal->warn("WoG file ZNPC00.TXT containing commander texts was not found");
		}
	}
}

int32_t CGeneralTextHandler::pluralText(const int32_t textIndex, const int32_t count) const
{
	if(textIndex == 0)
		return 0;
	else if(textIndex < 0)
		return -textIndex;
	else if(count == 1)
		return textIndex;
	else
		return textIndex + 1;
}

VCMI_LIB_NAMESPACE_END
