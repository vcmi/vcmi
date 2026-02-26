/*
 * CNonConstInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CNonConstInfoCallback.h"

#include "networkPacks/ArtifactLocation.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapObjects/IMarket.h"
#include "gameState/CGameState.h"
#include "mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

TerrainTile * CNonConstInfoCallback::getTile(const int3 & pos)
{
	if(!gameState().getMap().isInTheMap(pos))
		return nullptr;
	return &gameState().getMap().getTile(pos);
}

CGHeroInstance * CNonConstInfoCallback::getHero(const ObjectInstanceID & objid)
{
	return const_cast<CGHeroInstance*>(CGameInfoCallback::getHero(objid));
}

CGTownInstance * CNonConstInfoCallback::getTown(const ObjectInstanceID & objid)
{
	return const_cast<CGTownInstance*>(CGameInfoCallback::getTown(objid));
}

TeamState * CNonConstInfoCallback::getTeam(const TeamID & teamID)
{
	return const_cast<TeamState*>(CGameInfoCallback::getTeam(teamID));
}

TeamState * CNonConstInfoCallback::getPlayerTeam(const PlayerColor & color)
{
	return const_cast<TeamState*>(CGameInfoCallback::getPlayerTeam(color));
}

PlayerState * CNonConstInfoCallback::getPlayerState(const PlayerColor & color, bool verbose)
{
	return const_cast<PlayerState*>(CGameInfoCallback::getPlayerState(color, verbose));
}

CArtifactInstance * CNonConstInfoCallback::getArtInstance(const ArtifactInstanceID & aid)
{
	return gameState().getMap().getArtifactInstance(aid);
}

CGObjectInstance * CNonConstInfoCallback::getObjInstance(const ObjectInstanceID & oid)
{
	return gameState().getMap().getObject(oid);
}

CArmedInstance * CNonConstInfoCallback::getArmyInstance(const ObjectInstanceID & oid)
{
	return dynamic_cast<CArmedInstance *>(getObjInstance(oid));
}

CArtifactSet * CNonConstInfoCallback::getArtSet(const ArtifactLocation & loc)
{
	if(auto hero = getHero(loc.artHolder))
	{
		if(loc.creature.has_value())
		{
			if(loc.creature.value() == SlotID::COMMANDER_SLOT_PLACEHOLDER)
				return hero->getCommander();
			else
				return hero->getStackPtr(loc.creature.value());
		}
		else
		{
			return hero;
		}
	}
	else if(auto market = getMarket(loc.artHolder))
	{
		if(auto artSet = market->getArtifactsStorage())
			return artSet;
	}
	else if(auto army = getArmyInstance(loc.artHolder))
	{
		return army->getStackPtr(loc.creature.value());
	}
	return nullptr;
}

VCMI_LIB_NAMESPACE_END
