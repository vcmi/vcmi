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

#include "filesystem/Filesystem.h"
#include "serializer/JsonSerializeFormat.h"
#include "CConfigHandler.h"
#include "GameSettings.h"
#include "mapObjects/CQuest.h"
#include "modding/CModHandler.h"
#include "VCMI_Lib.h"
#include "Languages.h"
#include "TextOperations.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Detects language and encoding of H3 text files based on matching against pregenerated footprints of H3 file
void CGeneralTextHandler::detectInstallParameters()
{
	using LanguageFootprint = std::array<double, 16>;

	static const std::array<LanguageFootprint, 7> knownFootprints =
	{ {
		{ { 0.1602, 0.0000, 0.0357, 0.0054, 0.0038, 0.0017, 0.0077, 0.0214, 0.0000, 0.0000, 0.1264, 0.1947, 0.2012, 0.1406, 0.0480, 0.0532 } },
		{ { 0.0559, 0.0000, 0.1983, 0.0051, 0.0222, 0.0183, 0.4596, 0.2405, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000 } },
		{ { 0.0493, 0.0000, 0.1926, 0.0047, 0.0230, 0.0121, 0.4133, 0.2780, 0.0002, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0259, 0.0008 } },
		{ { 0.0534, 0.0000, 0.1705, 0.0047, 0.0418, 0.0208, 0.4775, 0.2191, 0.0001, 0.0000, 0.0000, 0.0000, 0.0000, 0.0005, 0.0036, 0.0080 } },
		{ { 0.0534, 0.0000, 0.1701, 0.0067, 0.0157, 0.0133, 0.4328, 0.2540, 0.0001, 0.0043, 0.0000, 0.0244, 0.0000, 0.0000, 0.0181, 0.0071 } },
		{ { 0.0548, 0.0000, 0.1744, 0.0061, 0.0031, 0.0009, 0.0046, 0.0136, 0.0000, 0.0004, 0.0000, 0.0000, 0.0227, 0.0061, 0.4882, 0.2252 } },
		{ { 0.0559, 0.0000, 0.1807, 0.0059, 0.0036, 0.0013, 0.0046, 0.0134, 0.0000, 0.0004, 0.0000, 0.0487, 0.0209, 0.0060, 0.4615, 0.1972 } },
	} };

	static const std::array<std::string, 7> knownLanguages =
	{ {
		"chinese",
		"english",
		"french",
		"german",
		"polish",
		"russian",
		"ukrainian"
	} };

	if(!CResourceHandler::get("core")->existsResource(TextPath::builtin("DATA/GENRLTXT.TXT")))
	{
		Settings language = settings.write["session"]["language"];
		language->String() = "english";

		Settings confidence = settings.write["session"]["languageDeviation"];
		confidence->Float() = 1.0;

		Settings encoding = settings.write["session"]["encoding"];
		encoding->String() = Languages::getLanguageOptions("english").encoding;

		return;
	}

	// load file that will be used for footprint generation
	// this is one of the most text-heavy files in game and consists solely from translated texts
	auto resource = CResourceHandler::get("core")->load(TextPath::builtin("DATA/GENRLTXT.TXT"));

	std::array<size_t, 256> charCount{};
	std::array<double, 16> footprint{};
	std::array<double, 6> deviations{};

	auto data = resource->readAll();

	// compute how often each character occurs in input file
	for (si64 i = 0; i < data.second; ++i)
		charCount[data.first[i]] += 1;

	// and convert computed data into weights
	// to reduce amount of data, group footprint data into 16-char blocks.
	// While this will reduce precision, it should not affect output
	// since we expect only tiny differences compared to reference footprints
	for (size_t i = 0; i < 256; ++i)
		footprint[i/16] += static_cast<double>(charCount[i]) / data.second;

	logGlobal->debug("Language footprint: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
			footprint[0], footprint[1], footprint[2],  footprint[3],  footprint[4],  footprint[5],  footprint[6],  footprint[7],
			footprint[8], footprint[9], footprint[10], footprint[11], footprint[12], footprint[13], footprint[14], footprint[15]
		);

	for (size_t i = 0; i < deviations.size(); ++i)
	{
		for (size_t j = 0; j < footprint.size(); ++j)
			deviations[i] += std::abs((footprint[j] - knownFootprints[i][j]));
	}

	size_t bestIndex = boost::range::min_element(deviations) - deviations.begin();

	for (size_t i = 0; i < deviations.size(); ++i)
		logGlobal->debug("Comparing to %s: %f", knownLanguages[i], deviations[i]);

	Settings language = settings.write["session"]["language"];
	language->String() = knownLanguages[bestIndex];

	Settings confidence = settings.write["session"]["languageDeviation"];
	confidence->Float() = deviations[bestIndex];

	Settings encoding = settings.write["session"]["encoding"];
	encoding->String() =  Languages::getLanguageOptions(knownLanguages[bestIndex]).encoding;
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

CLegacyConfigParser::CLegacyConfigParser(const TextPath & resource)
{
	auto input = CResourceHandler::get()->load(resource);
	std::string modName = VLC->modh->findResourceOrigin(resource);
	std::string language = VLC->modh->getModLanguage(modName);
	fileEncoding = Languages::getLanguageOptions(language).encoding;

	data.reset(new char[input->getSize()]);
	input->read(reinterpret_cast<uint8_t*>(data.get()), input->getSize());

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
	if (TextOperations::isValidASCII(str))
		return str;
	return TextOperations::toUnicode(str, fileEncoding);
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

void TextLocalizationContainer::registerStringOverride(const std::string & modContext, const std::string & language, const TextIdentifier & UID, const std::string & localized)
{
	assert(!modContext.empty());
	assert(!language.empty());

	// NOTE: implicitly creates entry, intended - strings added by maps, campaigns, vcmi and potentially - UI mods are not registered anywhere at the moment
	auto & entry = stringsLocalizations[UID.get()];

	entry.overrideLanguage = language;
	entry.overrideValue = localized;
	if (entry.modContext.empty())
		entry.modContext = modContext;
}

void TextLocalizationContainer::addSubContainer(const TextLocalizationContainer & container)
{
	subContainers.push_back(&container);
}

void TextLocalizationContainer::removeSubContainer(const TextLocalizationContainer & container)
{
	subContainers.erase(std::remove(subContainers.begin(), subContainers.end(), &container), subContainers.end());
}

const std::string & TextLocalizationContainer::deserialize(const TextIdentifier & identifier) const
{
	if(stringsLocalizations.count(identifier.get()) == 0)
	{
		for(auto containerIter = subContainers.rbegin(); containerIter != subContainers.rend(); ++containerIter)
			if((*containerIter)->identifierExists(identifier))
				return (*containerIter)->deserialize(identifier);
		
		logGlobal->error("Unable to find localization for string '%s'", identifier.get());
		return identifier.get();
	}

	const auto & entry = stringsLocalizations.at(identifier.get());

	if (!entry.overrideValue.empty())
		return entry.overrideValue;
	return entry.baseValue;
}

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized, const std::string & language)
{
	assert(!modContext.empty());
	assert(!Languages::getLanguageOptions(language).identifier.empty());
	assert(UID.get().find("..") == std::string::npos); // invalid identifier - there is section that was evaluated to empty string
	//assert(stringsLocalizations.count(UID.get()) == 0); // registering already registered string?

	if(stringsLocalizations.count(UID.get()) > 0)
	{
		auto & value = stringsLocalizations[UID.get()];
		value.baseLanguage = language;
		value.baseValue = localized;
	}
	else
	{
		StringState value;
		value.baseLanguage = language;
		value.baseValue = localized;
		value.modContext = modContext;

		stringsLocalizations[UID.get()] = value;
	}
}

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized)
{
	assert(!getModLanguage(modContext).empty());
	registerString(modContext, UID, localized, getModLanguage(modContext));
}

bool TextLocalizationContainer::validateTranslation(const std::string & language, const std::string & modContext, const JsonNode & config) const
{
	bool allPresent = true;

	for(const auto & string : stringsLocalizations)
	{
		if (string.second.modContext != modContext)
			continue; // Not our mod

		if (string.second.overrideLanguage == language)
			continue; // Already translated

		if (string.second.baseLanguage == language && !string.second.baseValue.empty())
			continue; // Base string already uses our language

		if (string.second.baseLanguage.empty())
			continue; // String added in localization, not present in base language (e.g. maps/campaigns)

		if (config.Struct().count(string.first) > 0)
			continue;

		if (allPresent)
			logMod->warn("Translation into language '%s' in mod '%s' is incomplete! Missing lines:", language, modContext);

		std::string currentText;
		if (string.second.overrideValue.empty())
			currentText = string.second.baseValue;
		else
			currentText = string.second.overrideValue;

		logMod->warn(R"(    "%s" : "%s",)", string.first, TextOperations::escapeString(currentText));
		allPresent = false;
	}

	bool allFound = true;

//	for(const auto & string : config.Struct())
//	{
//		if (stringsLocalizations.count(string.first) > 0)
//			continue;
//
//		if (allFound)
//			logMod->warn("Translation into language '%s' in mod '%s' has unused lines:", language, modContext);
//
//		logMod->warn(R"(    "%s" : "%s",)", string.first, TextOperations::escapeString(string.second.String()));
//		allFound = false;
//	}

	return allPresent && allFound;
}

void TextLocalizationContainer::loadTranslationOverrides(const std::string & language, const std::string & modContext, const JsonNode & config)
{
	for(const auto & node : config.Struct())
		registerStringOverride(modContext, language, node.first, node.second.String());
}

bool TextLocalizationContainer::identifierExists(const TextIdentifier & UID) const
{
	return stringsLocalizations.count(UID.get());
}

void TextLocalizationContainer::dumpAllTexts()
{
	logGlobal->info("BEGIN TEXT EXPORT");
	for(const auto & entry : stringsLocalizations)
	{
		if (!entry.second.overrideValue.empty())
			logGlobal->info(R"("%s" : "%s",)", entry.first, TextOperations::escapeString(entry.second.overrideValue));
		else
			logGlobal->info(R"("%s" : "%s",)", entry.first, TextOperations::escapeString(entry.second.baseValue));
	}

	logGlobal->info("END TEXT EXPORT");
}

std::string TextLocalizationContainer::getModLanguage(const std::string & modContext)
{
	if (modContext == "core")
		return CGeneralTextHandler::getInstalledLanguage();
	return VLC->modh->getModLanguage(modContext);
}

void TextLocalizationContainer::jsonSerialize(JsonNode & dest) const
{
	for(auto & s : stringsLocalizations)
	{
		dest.Struct()[s.first].String() = s.second.baseValue;
		if(!s.second.overrideValue.empty())
			dest.Struct()[s.first].String() = s.second.overrideValue;
	}
}

void CGeneralTextHandler::readToVector(const std::string & sourceID, const std::string & sourceName)
{
	CLegacyConfigParser parser(TextPath::builtin(sourceName));
	size_t index = 0;
	do
	{
		registerString( "core", {sourceID, index}, parser.readString());
		index += 1;
	}
	while (parser.endLine());
}

CGeneralTextHandler::CGeneralTextHandler():
	victoryConditions(*this, "core.vcdesc"   ),
	lossCondtions    (*this, "core.lcdesc"   ),
	colors           (*this, "core.plcolors" ),
	tcommands        (*this, "core.tcommand" ),
	hcommands        (*this, "core.hallinfo" ),
	fcommands        (*this, "core.castinfo" ),
	advobtxt         (*this, "core.advevent" ),
	restypes         (*this, "core.restypes" ),
	randsign         (*this, "core.randsign" ),
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
	readToVector("core.restypes", "DATA/RESTYPES.TXT" );
	readToVector("core.randsign", "DATA/RANDSIGN.TXT" );
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
	readToVector("core.xtrainfo", "DATA/XTRAINFO.TXT" );

	static const char * QE_MOD_COMMANDS = "DATA/QECOMMANDS.TXT";
	if (CResourceHandler::get()->existsResource(TextPath::builtin(QE_MOD_COMMANDS)))
		readToVector("vcmi.quickExchange", QE_MOD_COMMANDS);

	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/RANDTVRN.TXT"));
		parser.endLine();
		size_t index = 0;
		do
		{
			std::string line = parser.readString();
			if(!line.empty())
			{
				registerString("core", {"core.randtvrn", index}, line);
				index += 1;
			}
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/GENRLTXT.TXT"));
		parser.endLine();
		size_t index = 0;
		do
		{
			registerString("core", {"core.genrltxt", index}, parser.readString());
			index += 1;
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/HELP.TXT"));
		size_t index = 0;
		do
		{
			std::string first = parser.readString();
			std::string second = parser.readString();
			registerString("core", "core.help." + std::to_string(index) + ".hover", first);
			registerString("core", "core.help." + std::to_string(index) + ".help",  second);
			index += 1;
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/PLCOLORS.TXT"));
		size_t index = 0;
		do
		{
			std::string color = parser.readString();

			registerString("core", {"core.plcolors", index}, color);
			color[0] = toupper(color[0]);
			registerString("core", {"vcmi.capitalColors", index}, color);
			index += 1;
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/SEERHUT.TXT"));

		//skip header
		parser.endLine();

		for (size_t i = 0; i < 6; ++i)
		{
			registerString("core", {"core.seerhut.empty", i}, parser.readString());
		}
		parser.endLine();

		for (size_t i = 0; i < 9; ++i) //9 types of quests
		{
			std::string questName = CQuest::missionName(1+i);

			for (size_t j = 0; j < 5; ++j)
			{
				std::string questState = CQuest::missionState(j);

				parser.readString(); //front description
				for (size_t k = 0; k < 6; ++k)
				{
					registerString("core", {"core.seerhut.quest", questName, questState, k}, parser.readString());
				}
				parser.endLine();
			}
		}

		for (size_t k = 0; k < 6; ++k) //Time limit
		{
			registerString("core", {"core.seerhut.time", k}, parser.readString());
		}
		parser.endLine();

		parser.endLine(); // empty line
		parser.endLine(); // header

		for (size_t i = 0; i < 48; ++i)
		{
			registerString("core", {"core.seerhut.names", i}, parser.readString());
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/CAMPTEXT.TXT"));

		//skip header
		parser.endLine();

		std::string text;
		size_t campaignsCount = 0;
		do
		{
			text = parser.readString();
			if (!text.empty())
			{
				registerString("core", {"core.camptext.names", campaignsCount}, text);
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
					registerString("core", {"core.camptext.regions", std::to_string(campaign), region}, text);
					region += 1;
				}
			}
			while (parser.endLine() && !text.empty());

			scenariosCountPerCampaign.push_back(region);
		}
	}
	if (VLC->settings()->getBoolean(EGameSettings::MODULE_COMMANDERS))
	{
		if(CResourceHandler::get()->existsResource(TextPath::builtin("DATA/ZNPC00.TXT")))
			readToVector("vcmi.znpc00", "DATA/ZNPC00.TXT" );
	}
}

int32_t CGeneralTextHandler::pluralText(const int32_t textIndex, const int32_t count) const
{
	if(textIndex == 0)
		return 0;
	if(textIndex < 0)
		return -textIndex;
	if(count == 1)
		return textIndex;

	return textIndex + 1;
}

size_t CGeneralTextHandler::getCampaignLength(size_t campaignID) const
{
	assert(campaignID < scenariosCountPerCampaign.size());

	if(campaignID < scenariosCountPerCampaign.size())
		return scenariosCountPerCampaign[campaignID];
	return 0;
}

std::string CGeneralTextHandler::getPreferredLanguage()
{
	assert(!settings["general"]["language"].String().empty());
	return settings["general"]["language"].String();
}

std::string CGeneralTextHandler::getInstalledLanguage()
{
	assert(!settings["session"]["language"].String().empty());
	return settings["session"]["language"].String();
}

std::string CGeneralTextHandler::getInstalledEncoding()
{
	assert(!settings["session"]["encoding"].String().empty());
	return settings["session"]["encoding"].String();
}

std::vector<std::string> CGeneralTextHandler::findStringsWithPrefix(const std::string & prefix)
{
	std::vector<std::string> result;

	for(const auto & entry : stringsLocalizations)
	{
		if(boost::algorithm::starts_with(entry.first, prefix))
			result.push_back(entry.first);
	}

	return result;
}

LegacyTextContainer::LegacyTextContainer(CGeneralTextHandler & owner, std::string basePath):
	owner(owner),
	basePath(std::move(basePath))
{}

std::string LegacyTextContainer::operator[](size_t index) const
{
	return owner.translate(basePath, index);
}

LegacyHelpContainer::LegacyHelpContainer(CGeneralTextHandler & owner, std::string basePath):
	owner(owner),
	basePath(std::move(basePath))
{}

std::pair<std::string, std::string> LegacyHelpContainer::operator[](size_t index) const
{
	return {
		owner.translate(basePath + "." + std::to_string(index) + ".hover"),
		owner.translate(basePath + "." + std::to_string(index) + ".help")
	};
}

VCMI_LIB_NAMESPACE_END
