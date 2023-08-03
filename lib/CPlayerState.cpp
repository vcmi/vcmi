/*
 * CPlayerState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CPlayerState.h"
#include "gameState/QuestInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

PlayerState::PlayerState()
 : color(-1), human(false), enteredWinningCheatCode(false),
   enteredLosingCheatCode(false), status(EPlayerStatus::INGAME)
{
	setNodeType(PLAYER);
}

PlayerState::PlayerState(PlayerState && other) noexcept:
	CBonusSystemNode(std::move(other)),
	color(other.color),
	human(other.human),
	team(other.team),
	resources(other.resources),
	enteredWinningCheatCode(other.enteredWinningCheatCode),
	enteredLosingCheatCode(other.enteredLosingCheatCode),
	status(other.status),
	daysWithoutCastle(other.daysWithoutCastle)
{
	std::swap(visitedObjects, other.visitedObjects);
	std::swap(heroes, other.heroes);
	std::swap(towns, other.towns);
	std::swap(dwellings, other.dwellings);
	std::swap(quests, other.quests);
}

PlayerState::~PlayerState() = default;

std::string PlayerState::nodeName() const
{
	return "Player " + color.getStrCap(false);
}

PlayerColor PlayerState::getId() const
{
	return color;
}

int32_t PlayerState::getIndex() const
{
	return color.getNum();
}

int32_t PlayerState::getIconIndex() const 
{
	return color.getNum();
}
std::string PlayerState::getJsonKey() const
{
	return color.getStr(false);
}
std::string PlayerState::getNameTranslated() const
{
	return color.getStr(true);
}
std::string PlayerState::getNameTextID() const
{
	return color.getStr(false);
}
void PlayerState::registerIcons(const IconRegistar & cb) const
{
	//We cannot register new icons for players
}

TeamID PlayerState::getTeam() const
{
	return team;
}

bool PlayerState::isHuman() const
{
	return human;
}

const IBonusBearer * PlayerState::getBonusBearer() const
{
	return this;
}

int PlayerState::getResourceAmount(int type) const
{
	return vstd::atOrDefault(resources, static_cast<size_t>(type), 0);
}

VCMI_LIB_NAMESPACE_END
