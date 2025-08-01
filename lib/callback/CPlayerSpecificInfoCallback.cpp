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

#include "../gameState/CGameState.h"
#include "../gameState/QuestInfo.h"
#include "../CPlayerState.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"

//TODO make clean
#define ASSERT_IF_CALLED_WITH_PLAYER if(!getPlayerID()) {logGlobal->error(BOOST_CURRENT_FUNCTION); assert(0);}
#define ERROR_VERBOSE_OR_NOT_RET_VAL_IF(cond, verbose, txt, retVal) do {if(cond){if(verbose)logGlobal->error("%s: %s",BOOST_CURRENT_FUNCTION, txt); return retVal;}} while(0)
#define ERROR_RET_IF(cond, txt) do {if(cond){logGlobal->error("%s: %s", BOOST_CURRENT_FUNCTION, txt); return;}} while(0)
#define ERROR_RET_VAL_IF(cond, txt, retVal) do {if(cond){logGlobal->error("%s: %s", BOOST_CURRENT_FUNCTION, txt); return retVal;}} while(0)

VCMI_LIB_NAMESPACE_BEGIN

int CPlayerSpecificInfoCallback::howManyTowns() const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", -1);
	return CGameInfoCallback::howManyTowns(*getPlayerID());
}

std::vector < const CGTownInstance *> CPlayerSpecificInfoCallback::getTownsInfo(bool onlyOur) const
{
	auto ret = std::vector < const CGTownInstance *>();
	for(const auto & i : gameState().players)
	{
		for(const auto & town : i.second.getTowns())
		{
			if(i.first == getPlayerID() || (!onlyOur && isVisibleFor(town, *getPlayerID())))
			{
				ret.push_back(town);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gameState().players.begin() ; i!=gameState().players.end();i++)
	return ret;
}
std::vector < const CGHeroInstance *> CPlayerSpecificInfoCallback::getHeroesInfo() const
{
	const auto * playerState = gameState().getPlayerState(*getPlayerID());
	return playerState->getHeroes();
}

int CPlayerSpecificInfoCallback::getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned) const
{
	if (hero->isGarrisoned() && !includeGarrisoned)
		return -1;

	size_t index = 0;
	const auto & heroes = gameState().players.at(*getPlayerID()).getHeroes();

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
		TeamID t = gameState().getPlayerTeam(*getPlayerID())->id;
		double visited = 0.0;
		if(gameState().getMap().obelisksVisited.count(t))
			visited = static_cast<double>(gameState().getMap().obelisksVisited[t]);

		*outKnownRatio = visited / gameState().getMap().obeliskCount;
	}
	return gameState().getMap().grailPos;
}

std::vector < const CGObjectInstance * > CPlayerSpecificInfoCallback::getMyObjects() const
{
	return gameState().getPlayerState(*getPlayerID())->getOwnedObjects();
}

std::vector <QuestInfo> CPlayerSpecificInfoCallback::getMyQuests() const
{
	return gameState().getPlayerState(*getPlayerID())->quests;
}

int CPlayerSpecificInfoCallback::howManyHeroes(bool includeGarrisoned) const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", -1);
	return getHeroCount(*getPlayerID(), includeGarrisoned);
}

const CGHeroInstance* CPlayerSpecificInfoCallback::getHeroBySerial(int serialId, bool includeGarrisoned) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
		const PlayerState *p = getPlayerState(*getPlayerID());
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);

	if (!includeGarrisoned)
	{
		for(ui32 i = 0; i < p->getHeroes().size() && static_cast<int>(i) <= serialId; i++)
			if(p->getHeroes()[i]->isGarrisoned())
				serialId++;
	}
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->getHeroes().size(), "No player info", nullptr);
	return p->getHeroes()[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
		const PlayerState *p = getPlayerState(*getPlayerID());
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->getTowns().size(), "No player info", nullptr);
	return p->getTowns()[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(GameResID type) const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", -1);
	return getResource(*getPlayerID(), type);
}

TResources CPlayerSpecificInfoCallback::getResourceAmount() const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", TResources());
	return gameState().players.at(*getPlayerID()).resources;
}

VCMI_LIB_NAMESPACE_END
