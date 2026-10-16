/*
 * CPlayerSpecificInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPlayerSpecificInfoCallback.h"

#include "CallbackDefines.h"

#include "../gameState/CGameState.h"
#include "../gameState/QuestInfo.h"
#include "../CPlayerState.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"

//TODO make clean

int CPlayerSpecificInfoCallback::howManyTowns() const
{
	THROW_IF(!getPlayerID(), "Applicable only for player callbacks");
	return CGameInfoCallback::howManyTowns(getPlayerID().value());
}

std::vector < const CGTownInstance *> CPlayerSpecificInfoCallback::getTownsInfo(bool onlyOur) const
{
	auto ret = std::vector < const CGTownInstance *>();
	for(const auto & i : gameState().players)
	{
		for(const auto & town : i.second.getTowns())
		{
			if(i.first == getPlayerID() || (!onlyOur && isVisibleFor(town, getPlayerID().value())))
			{
				ret.push_back(town);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gameState().players.begin() ; i!=gameState().players.end();i++)
	return ret;
}
std::vector < const CGHeroInstance *> CPlayerSpecificInfoCallback::getHeroesInfo() const
{
	const auto * playerState = gameState().getPlayerState(getPlayerID().value());
	return playerState->getHeroes();
}

int CPlayerSpecificInfoCallback::getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned) const
{
	if (hero->isGarrisoned() && !includeGarrisoned)
		return -1;

	size_t index = 0;
	const auto & heroes = gameState().players.at(getPlayerID().value()).getHeroes();

	for (auto & possibleHero : heroes)
	{
		if (includeGarrisoned || !possibleHero->isGarrisoned())
			index++;

		if (possibleHero == hero)
			return static_cast<int>(index);
	}
	return -1;
}

int3 CPlayerSpecificInfoCallback::getGrailPos( double *outKnownRatio )
{
	if (!getPlayerID() || gameState().getMap().obeliskCount == 0)
	{
		*outKnownRatio = 0.0;
	}
	else
	{
		TeamID t = gameState().getPlayerTeam(getPlayerID().value())->id;
		double visited = 0.0;
		if(gameState().getMap().obelisksVisited.count(t))
			visited = static_cast<double>(gameState().getMap().obelisksVisited[t]);

		*outKnownRatio = visited / gameState().getMap().obeliskCount;
	}
	return gameState().getMap().grailPos;
}

std::vector < const CGObjectInstance * > CPlayerSpecificInfoCallback::getMyObjects() const
{
	return gameState().getPlayerState(getPlayerID().value())->getOwnedObjects();
}

std::vector <QuestInfo> CPlayerSpecificInfoCallback::getMyQuests() const
{
	return gameState().getPlayerState(getPlayerID().value())->quests;
}

std::vector<ScenarioEventJournalEntry> CPlayerSpecificInfoCallback::getMyScenarioEventJournal() const
{
	return gameState().getPlayerState(getPlayerID().value())->scenarioEventJournal;
}

int CPlayerSpecificInfoCallback::howManyHeroes(bool includeGarrisoned) const
{
	THROW_IF(!getPlayerID(), "Applicable only for player callbacks");
	return getHeroCount(getPlayerID().value(), includeGarrisoned);
}

const CGHeroInstance* CPlayerSpecificInfoCallback::getHeroBySerial(int serialId, bool includeGarrisoned) const
{
	THROW_IF_CALLED_WITH_PLAYER();
	const PlayerState *p = getPlayerState(getPlayerID().value());
	THROW_IF(!p, "No player info");

	if (!includeGarrisoned)
	{
		for(ui32 i = 0; i < p->getHeroes().size() && static_cast<int>(i) <= serialId; i++)
			if(p->getHeroes()[i]->isGarrisoned())
				serialId++;
	}
	THROW_IF(serialId < 0 || serialId >= p->getHeroes().size(), "No player info");
	return p->getHeroes()[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	THROW_IF_CALLED_WITH_PLAYER();
	const PlayerState *p = getPlayerState(getPlayerID().value());
	THROW_IF(!p, "No player info");
	THROW_IF(serialId < 0 || serialId >= p->getTowns().size(), "No player info");
	return p->getTowns()[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(GameResID type) const
{
	THROW_IF(!getPlayerID(), "Applicable only for player callbacks");
	return getResource(getPlayerID().value(), type);
}

TResources CPlayerSpecificInfoCallback::getResourceAmount() const
{
	THROW_IF(!getPlayerID(), "Applicable only for player callbacks");
	return gameState().players.at(getPlayerID().value()).resources;
}
