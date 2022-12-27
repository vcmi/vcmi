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
#include "mapObjects/CQuest.h"
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

void CGeneralTextHandler::readToVector(std::string sourceID, std::string sourceName)
{
	CLegacyConfigParser parser(sourceName);
	size_t index = 0;
	do
	{
		registerH3String(sourceID, index, parser.readString());
		index += 1;
	}
	while (parser.endLine());
}

const std::string & CGeneralTextHandler::translate(const std::string & identifier, size_t index) const
{
	return translate(identifier + std::to_string(index));
}

const std::string & CGeneralTextHandler::translate(const std::string & identifier) const
{
	return deserialize(identifier);
}

const std::string & CGeneralTextHandler::serialize(const std::string & identifier) const
{
	assert(stringsIdentifiers.count(identifier));
	return stringsIdentifiers.at(identifier);
}

const std::string & CGeneralTextHandler::deserialize(const std::string & identifier) const
{
	if (stringsLocalizations.count(identifier))
		return stringsLocalizations.at(identifier);
	logGlobal->error("Unable to find localization for string '%s'", identifier);
	return identifier;
}

void CGeneralTextHandler::registerH3String(const std::string & file, size_t index, const std::string & localized)
{
	registerString(file + '.' + std::to_string(index), localized);
}

void CGeneralTextHandler::registerString(const std::string & UID, const std::string & localized)
{
	stringsIdentifiers[localized] = UID;
	stringsLocalizations[UID] = localized;
}

CGeneralTextHandler::CGeneralTextHandler():
	victoryConditions(*this, "core.vcdesc"   ),
	lossCondtions    (*this, "core.lcdesc"   ),
	colors           (*this, "core.plcolors" ),
	tcommands        (*this, "core.tcommand" ),
	hcommands        (*this, "core.hallinfo" ),
	fcommands        (*this, "core.castinfo" ),
	advobtxt         (*this, "core.advevent" ),
	xtrainfo         (*this, "core.xtrainfo" ),
	restypes         (*this, "core.restypes" ),
	terrainNames     (*this, "core.terrname" ),
	randsign         (*this, "core.randsign" ),
	creGens          (*this, "core.crgen1"   ),
	creGens4         (*this, "core.crgen4"   ),
	overview         (*this, "core.overview" ),
	arraytxt         (*this, "core.arraytxt" ),
	primarySkillNames(*this, "core.priskill" ),
	jktexts          (*this, "core.jktext"   ),
	tavernInfo       (*this, "core.tvrninfo" ),
	tavernRumors     (*this, "core.randtvrn" ),
	turnDurations    (*this, "core.turndur"  ),
	heroscrn         (*this, "core.heroscrn" ),
	tentColors       (*this, "core.tentcolr" ),
	levels           (*this, "core.skilllev" ),
	zelp             (*this, "core.help"     ),
	allTexts         (*this, "core.genrltxt" ),
	// pseudo-array, that don't have H3 file with same name
	seerEmpty        (*this, "core.seerhut.empty"  ),
	seerNames        (*this, "core.seerhut.names"  ),
	capColors        (*this, "vcmi.capitalColors"  ),
	znpc00           (*this, "vcmi.znpc00"  ), // technically - wog
	qeModCommands    (*this, "vcmi.quickExchange" )
{
	readToVector("core.vcdesc",   "DATA/VCDESC.TXT"   );
	readToVector("core.lcdesc",   "DATA/LCDESC.TXT"   );
	readToVector("core.tcommand", "DATA/TCOMMAND.TXT" );
	readToVector("core.hallinfo", "DATA/HALLINFO.TXT" );
	readToVector("core.castinfo", "DATA/CASTINFO.TXT" );
	readToVector("core.advevent", "DATA/ADVEVENT.TXT" );
	readToVector("core.xtrainfo", "DATA/XTRAINFO.TXT" );
	readToVector("core.restypes", "DATA/RESTYPES.TXT" );
	readToVector("core.terrname", "DATA/TERRNAME.TXT" );
	readToVector("core.randsign", "DATA/RANDSIGN.TXT" );
	readToVector("core.crgen1",   "DATA/CRGEN1.TXT"   );
	readToVector("core.crgen4",   "DATA/CRGEN4.TXT"   );
	readToVector("core.overview", "DATA/OVERVIEW.TXT" );
	readToVector("core.arraytxt", "DATA/ARRAYTXT.TXT" );
	readToVector("core.priskill", "DATA/PRISKILL.TXT" );
	readToVector("core.jktext",   "DATA/JKTEXT.TXT"   );
	readToVector("core.tvrninfo", "DATA/TVRNINFO.TXT" );
	readToVector("core.turndur",  "DATA/TURNDUR.TXT"  );
	readToVector("core.heroscrn", "DATA/HEROSCRN.TXT" );
	readToVector("core.tentcolr", "DATA/TENTCOLR.TXT" );
	readToVector("core.skilllev", "DATA/SKILLLEV.TXT" );
	readToVector("core.cmpmusic", "DATA/CMPMUSIC.TXT" );
	readToVector("core.minename", "DATA/MINENAME.TXT" );
	readToVector("core.mineevnt", "DATA/MINEEVNT.TXT" );

	static const char * QE_MOD_COMMANDS = "DATA/QECOMMANDS.TXT";
	if (CResourceHandler::get()->existsResource(ResourceID(QE_MOD_COMMANDS, EResType::TEXT)))
		readToVector("vcmi.quickExchange", QE_MOD_COMMANDS);

	auto vcmiTexts = JsonNode(ResourceID("config/translate.json", EResType::TEXT));

	for ( auto const & node : vcmiTexts.Struct())
		registerString(node.first, node.second.String());

	{
		CLegacyConfigParser parser("DATA/RANDTVRN.TXT");
		parser.endLine();
		size_t index = 0;
		do
		{
			std::string line = parser.readString();
			if (!line.empty())
			{
				registerH3String("core.randtvrn", index, line);
				index += 1;
			}
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/GENRLTXT.TXT");
		parser.endLine();
		size_t index = 0;
		do
		{
			registerH3String("core.genrltxt", index, parser.readString());
			index += 1;
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/HELP.TXT");
		size_t index = 0;
		do
		{
			std::string first = parser.readString();
			std::string second = parser.readString();
			registerString("core.help." + std::to_string(index) + ".label", first);
			registerString("core.help." + std::to_string(index) + ".help",  second);
			index += 1;
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/PLCOLORS.TXT");
		size_t index = 0;
		do
		{
			std::string color = parser.readString();

			registerH3String("core.plcolors", index, color);
			color[0] = toupper(color[0]);
			registerH3String("vcmi.capitalColors", index, color);
			index += 1;
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/SEERHUT.TXT");

		//skip header
		parser.endLine();

		for (int i = 0; i < 6; ++i)
		{
			registerH3String("core.seerhut.empty", i, parser.readString());
		}
		parser.endLine();

		for (int i = 0; i < 9; ++i) //9 types of quests
		{
			std::string questName = CQuest::missionName(CQuest::Emission(1+i));

			for (int j = 0; j < 5; ++j)
			{
				std::string questState = CQuest::missionState(j);

				parser.readString(); //front description
				for (int k = 0; k < 6; ++k)
				{
					registerH3String("core.seerhut.quest." + questName + "." + questState, k, parser.readString());
				}
				parser.endLine();
			}
		}

		for (int k = 0; k < 6; ++k) //Time limit
		{
			registerH3String("core.seerhut.time", k, parser.readString());
		}
		parser.endLine();

		parser.endLine(); // empty line
		parser.endLine(); // header

		for (int i = 0; i < 48; ++i)
		{
			registerH3String("core.seerhut.names", i, parser.readString());
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/CAMPTEXT.TXT");

		//skip header
		parser.endLine();

		std::string text;
		size_t campaignsCount = 0;
		do
		{
			text = parser.readString();
			if (!text.empty())
			{
				registerH3String("core.camptext.names", campaignsCount, text);
				campaignsCount += 1;
			}
		}
		while (parser.endLine() && !text.empty());

		for (size_t campaign=0; campaign<campaignsCount; campaign++)
		{
			size_t region = 0;

			do // skip empty space and header
			{
				text = parser.readString();
			}
			while (parser.endLine() && text.empty());

			do
			{
				text = parser.readString();
				if (!text.empty())
				{
					registerH3String("core.camptext.regions." + std::to_string(campaign), region, text);
					region += 1;
				}
			}
			while (parser.endLine() && !text.empty());

			scenariosCountPerCampaign.push_back(region);
		}
	}
	if (VLC->modh->modules.COMMANDERS)
	{
		if (CResourceHandler::get()->existsResource(ResourceID("DATA/ZNPC00.TXT", EResType::TEXT)))
			readToVector("vcmi.znpc00", "DATA/ZNPC00.TXT" );
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

void CGeneralTextHandler::dumpAllTexts()
{
	logGlobal->info("BEGIN TEXT EXPORT");
	for ( auto const & entry : stringsLocalizations)
	{
		auto cleanString = entry.second;
		boost::replace_all(cleanString, "\\", "\\\\");
		boost::replace_all(cleanString, "\n", "\\n");
		boost::replace_all(cleanString, "\r", "\\r");
		boost::replace_all(cleanString, "\t", "\\t");
		boost::replace_all(cleanString, "\"", "\\\"");

		logGlobal->info("\"%s\" : \"%s\",", entry.first, cleanString);
	}
	logGlobal->info("END TEXT EXPORT");
}

size_t CGeneralTextHandler::getCampaignLength(size_t campaignID) const
{
	assert(campaignID < scenariosCountPerCampaign.size());

	if ( campaignID < scenariosCountPerCampaign.size())
		return scenariosCountPerCampaign[campaignID];
	return 0;
}

std::vector<std::string> CGeneralTextHandler::findStringsWithPrefix(std::string const & prefix)
{
	std::vector<std::string> result;

	for (auto const & entry : stringsLocalizations)
	{
		if (boost::algorithm::starts_with(entry.first, prefix))
			result.push_back(entry.first);
	}

	return result;
}

LegacyTextContainer::LegacyTextContainer(CGeneralTextHandler & owner, std::string const & basePath):
	owner(owner),
	basePath(basePath)
{}

const std::string & LegacyTextContainer::operator[](size_t index) const
{
	return owner.translate(basePath + "." + std::to_string(index));
}

LegacyHelpContainer::LegacyHelpContainer(CGeneralTextHandler & owner, std::string const & basePath):
	owner(owner),
	basePath(basePath)
{}

std::pair<std::string, std::string> LegacyHelpContainer::operator[](size_t index) const
{
	return {
		owner.translate(basePath + "." + std::to_string(index) + ".label"),
		owner.translate(basePath + "." + std::to_string(index) + ".help")
	};
}


VCMI_LIB_NAMESPACE_END
