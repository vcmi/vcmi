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
#include "texts/CGeneralTextHandler.h"

#include "CLegacyConfigParser.h"
#include "CConfigHandler.h"
#include "GameSettings.h"
#include "Languages.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjects/CQuest.h"

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
	lossConditions    (*this, "core.lcdesc"   ),
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
	readToVector("core.plcolors", "DATA/PLCOLORS.TXT" );
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

	static const std::string QE_MOD_COMMANDS = "DATA/QECOMMANDS.TXT";
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
			EQuestMission missionID = static_cast<EQuestMission>(i+1);

			std::string questName = CQuest::missionName(missionID);

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
	std::lock_guard globalLock(globalTextMutex);
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
