/*
 * CMapHeader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapHeader.h"

#include "MapFormat.h"

#include "../GameLibrary.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../json/JsonUtils.h"
#include "../modding/CModHandler.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/Languages.h"

VCMI_LIB_NAMESPACE_BEGIN

SHeroName::SHeroName() : heroId(-1)
{
}

PlayerInfo::PlayerInfo(): canHumanPlay(false), canComputerPlay(false),
	aiTactic(EAiTactic::RANDOM), isFactionRandom(false), hasRandomHero(false), mainCustomHeroPortrait(-1), mainCustomHeroId(-1), hasMainTown(false),
	generateHeroAtMainTown(false), posOfMainTown(-1), team(TeamID::NO_TEAM)
{
	allowedFactions = LIBRARY->townh->getAllowedFactions();
}

FactionID PlayerInfo::defaultCastle() const
{
	//if random allowed set it as default
	if(isFactionRandom)
		return FactionID::RANDOM;

	assert(!allowedFactions.empty());
	if(!allowedFactions.empty())
		return *allowedFactions.begin();

	// fall back to random
	return FactionID::RANDOM;
}

HeroTypeID PlayerInfo::defaultHero() const
{
	// we will generate hero in front of main town
	if((generateHeroAtMainTown && hasMainTown) || hasRandomHero)
	{
		//random hero
		return HeroTypeID::RANDOM;
	}

	return HeroTypeID::NONE;
}

bool PlayerInfo::canAnyonePlay() const
{
	return canHumanPlay || canComputerPlay;
}

bool PlayerInfo::hasCustomMainHero() const
{
	return mainCustomHeroId.isValid();
}

EventCondition::EventCondition(EWinLoseType condition):
	value(-1),
	position(-1, -1, -1),
	condition(condition)
{
}

EventCondition::EventCondition(EWinLoseType condition, si32 value, TargetTypeID objectType, const int3 & position):
	value(value),
	objectType(objectType),
	position(position),
	condition(condition)
{}


void CMapHeader::setupEvents()
{
	EventCondition victoryCondition(EventCondition::STANDARD_WIN);
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	//Victory condition - defeat all
	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage.appendTextID("core.genrltxt.5");
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill.appendTextID("core.genrltxt.659");
	standardVictory.trigger = EventExpression(victoryCondition);

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage.appendTextID("core.genrltxt.8");
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill.appendTextID("core.genrltxt.7");
	standardDefeat.trigger = EventExpression(defeatCondition);

	triggeredEvents.push_back(standardVictory);
	triggeredEvents.push_back(standardDefeat);

	victoryIconIndex = 11;
	victoryMessage.appendTextID("core.vcdesc.0");

	defeatIconIndex = 3;
	defeatMessage.appendTextID("core.lcdesc.0");
}

CMapHeader::CMapHeader()
	: version(EMapFormat::VCMI)
	, height(72)
	, width(72)
	, mapLevels(2)
	, difficulty(EMapDifficulty::NORMAL)
	, levelLimit(0)
	, victoryIconIndex(0)
	, defeatIconIndex(0)
	, howManyTeams(0)
	, areAnyPlayers(false)
	, battleOnly(false)
{
	setupEvents();
	allowedHeroes = LIBRARY->heroh->getDefaultAllowed();
	players.resize(PlayerColor::PLAYER_LIMIT_I);
}

CMapHeader::~CMapHeader() = default;

ui8 CMapHeader::levels() const
{
	return mapLevels;
}

void CMapHeader::registerMapStrings()
{
	//get supported languages. Assuming that translation containing most strings is the base language
	std::set<std::string, std::less<>> mapLanguages;
	std::set<std::string, std::less<>> mapBaseLanguages;
	int maxStrings = 0;
	for(auto & translation : translations.Struct())
	{
		if(translation.first.empty() || !translation.second.isStruct() || translation.second.Struct().empty())
			continue;
		
		if(translation.second.Struct().size() > maxStrings)
			maxStrings = translation.second.Struct().size();
		mapLanguages.insert(translation.first);
	}
	
	if(maxStrings == 0 || mapLanguages.empty())
	{
		logGlobal->trace("Map %s doesn't have any supported translation", name.toString());
		return;
	}
	
	//identifying base languages
	for(auto & translation : translations.Struct())
	{
		if(translation.second.isStruct() && translation.second.Struct().size() == maxStrings)
			mapBaseLanguages.insert(translation.first);
	}
	
	std::string baseLanguage;
	std::string language;
	//english is preferable as base language
	if(mapBaseLanguages.count(Languages::getLanguageOptions(Languages::ELanguages::ENGLISH).identifier))
		baseLanguage = Languages::getLanguageOptions(Languages::ELanguages::ENGLISH).identifier;
	else
		baseLanguage = *mapBaseLanguages.begin();

	if(mapBaseLanguages.count(CGeneralTextHandler::getPreferredLanguage()))
	{
		language = CGeneralTextHandler::getPreferredLanguage(); //preferred language is base language - use it
		baseLanguage = language;
	}
	else
	{
		if(mapLanguages.count(CGeneralTextHandler::getPreferredLanguage()))
			language = CGeneralTextHandler::getPreferredLanguage();
		else
			language = baseLanguage; //preferred language is not supported, use base language
	}
	
	assert(!language.empty());
	
	JsonNode data = translations[baseLanguage];
	if(language != baseLanguage)
		JsonUtils::mergeCopy(data, translations[language]);
	
	for(auto & s : data.Struct())
		texts.registerString("map", TextIdentifier(s.first), s.second.String());
}

std::string mapRegisterLocalizedString(const std::string & modContext, CMapHeader & mapHeader, const TextIdentifier & UID, const std::string & localized)
{
	return mapRegisterLocalizedString(modContext, mapHeader, UID, localized, LIBRARY->modh->getModLanguage(modContext));
}

std::string mapRegisterLocalizedString(const std::string & modContext, CMapHeader & mapHeader, const TextIdentifier & UID, const std::string & localized, const std::string & language)
{
	mapHeader.texts.registerString(modContext, UID, localized);
	mapHeader.translations.Struct()[language].Struct()[UID.get()].String() = localized;
	return UID.get();
}

VCMI_LIB_NAMESPACE_END
