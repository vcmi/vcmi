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
#include "CGameStateFwd.h"

VCMI_LIB_NAMESPACE_BEGIN

PlayerState::PlayerState()
 : color(-1), human(false), enteredWinningCheatCode(false),
   enteredLosingCheatCode(false), status(EPlayerStatus::INGAME)
{
	setNodeType(PLAYER);
}

PlayerState::PlayerState(PlayerState && other):
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
	std::swap(availableHeroes, other.availableHeroes);
	std::swap(dwellings, other.dwellings);
	std::swap(quests, other.quests);
}

std::string PlayerState::nodeName() const
{
	return "Player " + color.getStrCap(false);
}

PlayerColor PlayerState::getColor() const
{
	return color;
}

TeamID PlayerState::getTeam() const
{
	return team;
}

bool PlayerState::isHuman() const
{
	return human;
}

const IBonusBearer * PlayerState::accessBonuses() const
{
	return this;
}

int PlayerState::getResourceAmount(int type) const
{
	return vstd::atOrDefault(resources, static_cast<size_t>(type), 0);
}

VCMI_LIB_NAMESPACE_END
