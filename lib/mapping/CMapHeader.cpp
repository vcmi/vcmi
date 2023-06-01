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

#include "../VCMI_Lib.h"
#include "../CTownHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

SHeroName::SHeroName() : heroId(-1)
{
}

PlayerInfo::PlayerInfo(): canHumanPlay(false), canComputerPlay(false),
	aiTactic(EAiTactic::RANDOM), isFactionRandom(false), hasRandomHero(false), mainCustomHeroPortrait(-1), mainCustomHeroId(-1), hasMainTown(false),
	generateHeroAtMainTown(false), posOfMainTown(-1), team(TeamID::NO_TEAM)
{
	allowedFactions = VLC->townh->getAllowedFactions();
}

si8 PlayerInfo::defaultCastle() const
{
	//if random allowed set it as default
	if(isFactionRandom)
		return -1;

	if(!allowedFactions.empty())
		return *allowedFactions.begin();

	// fall back to random
	return -1;
}

si8 PlayerInfo::defaultHero() const
{
	// we will generate hero in front of main town
	if((generateHeroAtMainTown && hasMainTown) || hasRandomHero)
	{
		//random hero
		return -1;
	}

	return -2;
}

bool PlayerInfo::canAnyonePlay() const
{
	return canHumanPlay || canComputerPlay;
}

bool PlayerInfo::hasCustomMainHero() const
{
	return !mainCustomHeroName.empty() && mainCustomHeroPortrait != -1;
}

EventCondition::EventCondition(EWinLoseType condition):
	object(nullptr),
	metaType(EMetaclass::INVALID),
	value(-1),
	objectType(-1),
	objectSubtype(-1),
	position(-1, -1, -1),
	condition(condition)
{
}

EventCondition::EventCondition(EWinLoseType condition, si32 value, si32 objectType, const int3 & position):
	object(nullptr),
	metaType(EMetaclass::INVALID),
	value(value),
	objectType(objectType),
	objectSubtype(-1),
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
	standardVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[5];
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill = VLC->generaltexth->allTexts[659];
	standardVictory.trigger = EventExpression(victoryCondition);

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage = VLC->generaltexth->allTexts[8];
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill = VLC->generaltexth->allTexts[7];
	standardDefeat.trigger = EventExpression(defeatCondition);

	triggeredEvents.push_back(standardVictory);
	triggeredEvents.push_back(standardDefeat);

	victoryIconIndex = 11;
	victoryMessage = VLC->generaltexth->victoryConditions[0];

	defeatIconIndex = 3;
	defeatMessage = VLC->generaltexth->lossCondtions[0];
}

CMapHeader::CMapHeader() : version(EMapFormat::VCMI), height(72), width(72),
	twoLevel(true), difficulty(1), levelLimit(0), howManyTeams(0), areAnyPlayers(false)
{
	setupEvents();
	allowedHeroes = VLC->heroh->getDefaultAllowed();
	players.resize(PlayerColor::PLAYER_LIMIT_I);
}

ui8 CMapHeader::levels() const
{
	return (twoLevel ? 2 : 1);
}

VCMI_LIB_NAMESPACE_END
