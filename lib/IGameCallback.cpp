/*
 * IGameCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "IGameCallback.h"

#include "spells/CSpellHandler.h"// for CSpell
#include "CSkillHandler.h"// for CSkill
#include "CBonusTypeHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "bonuses/Limiters.h"
#include "bonuses/Propagators.h"
#include "bonuses/Updaters.h"
#include "entities/artifact/CArtifact.h"
#include "entities/building/CBuilding.h"
#include "entities/hero/CHero.h"
#include "networkPacks/ArtifactLocation.h"
#include "serializer/CLoadFile.h"
#include "rmg/CMapGenOptions.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "mapObjects/CGMarket.h"
#include "mapObjects/TownBuildingInstance.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapObjects/CGTownInstance.h"
#include "mapObjects/CObjectHandler.h"
#include "mapObjects/CQuest.h"
#include "mapObjects/MiscObjects.h"
#include "mapObjects/ObjectTemplate.h"
#include "campaign/CampaignState.h"
#include "StartInfo.h"
#include "gameState/CGameState.h"
#include "gameState/CGameStateCampaign.h"
#include "gameState/TavernHeroesPool.h"
#include "gameState/QuestInfo.h"
#include "mapping/CMap.h"
#include "modding/CModHandler.h"
#include "modding/IdentifierStorage.h"
#include "modding/CModVersion.h"
#include "modding/ActiveModsInSaveList.h"
#include "CPlayerState.h"
#include "GameSettings.h"
#include "ScriptHandler.h"
#include "RoadHandler.h"
#include "RiverHandler.h"
#include "TerrainHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void CPrivilegedInfoCallback::getFreeTiles(std::vector<int3> & tiles) const
{
	std::vector<int> floors;
	floors.reserve(gameState().getMap().levels());
	for(int b = 0; b < gameState().getMap().levels(); ++b)
	{
		floors.push_back(b);
	}
	const TerrainTile * tinfo = nullptr;
	for (auto zd : floors)
	{
		for (int xd = 0; xd < gameState().getMap().width; xd++)
		{
			for (int yd = 0; yd < gameState().getMap().height; yd++)
			{
				tinfo = getTile(int3 (xd,yd,zd));
				if (tinfo->isLand() && tinfo->getTerrain()->isPassable() && !tinfo->blocked()) //land and free
					tiles.emplace_back(xd, yd, zd);
			}
		}
	}
}

void CPrivilegedInfoCallback::getTilesInRange(std::unordered_set<int3> & tiles,
											  const int3 & pos,
											  int radious,
											  ETileVisibility mode,
											  std::optional<PlayerColor> player,
											  int3::EDistanceFormula distanceFormula) const
{
	if(!!player && !player->isValidPlayer())
	{
		logGlobal->error("Illegal call to getTilesInRange!");
		return;
	}
	if(radious == CBuilding::HEIGHT_SKYSHIP) //reveal entire map
		getAllTiles (tiles, player, -1, [](auto * tile){return true;});
	else
	{
		const TeamState * team = !player ? nullptr : gameState().getPlayerTeam(*player);
		for (int xd = std::max<int>(pos.x - radious , 0); xd <= std::min<int>(pos.x + radious, gameState().getMap().width - 1); xd++)
		{
			for (int yd = std::max<int>(pos.y - radious, 0); yd <= std::min<int>(pos.y + radious, gameState().getMap().height - 1); yd++)
			{
				int3 tilePos(xd,yd,pos.z);
				int distance = pos.dist(tilePos, distanceFormula);

				if(distance <= radious)
				{
					if(!player
						|| (mode == ETileVisibility::HIDDEN  && team->fogOfWarMap[pos.z][xd][yd] == 0)
						|| (mode == ETileVisibility::REVEALED && team->fogOfWarMap[pos.z][xd][yd] == 1)
					)
						tiles.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
}

void CPrivilegedInfoCallback::getAllTiles(std::unordered_set<int3> & tiles, std::optional<PlayerColor> Player, int level, std::function<bool(const TerrainTile *)> filter) const
{
	if(!!Player && !Player->isValidPlayer())
	{
		logGlobal->error("Illegal call to getAllTiles !");
		return;
	}

	std::vector<int> floors;
	if(level == -1)
	{
		for(int b = 0; b < gameState().getMap().levels(); ++b)
		{
			floors.push_back(b);
		}
	}
	else
		floors.push_back(level);

	for(auto zd: floors)
	{
		for(int xd = 0; xd < gameState().getMap().width; xd++)
		{
			for(int yd = 0; yd < gameState().getMap().height; yd++)
			{
				int3 coordinates(xd, yd, zd);
				if (filter(getTile(coordinates)))
					tiles.insert(coordinates);
			}
		}
	}
}

void CPrivilegedInfoCallback::pickAllowedArtsSet(std::vector<ArtifactID> & out, vstd::RNG & rand)
{
	for (int j = 0; j < 3 ; j++)
		out.push_back(gameState().pickRandomArtifact(rand, EArtifactClass::ART_TREASURE));
	for (int j = 0; j < 3 ; j++)
		out.push_back(gameState().pickRandomArtifact(rand, EArtifactClass::ART_MINOR));

	out.push_back(gameState().pickRandomArtifact(rand, EArtifactClass::ART_MAJOR));
}

void CPrivilegedInfoCallback::getAllowedSpells(std::vector<SpellID> & out, std::optional<ui16> level)
{
	for (auto const & spellID : gameState().getMap().allowedSpells)
	{
		const auto * spell = spellID.toEntity(LIBRARY);

		if (!isAllowed(spellID))
			continue;

		if (level.has_value() && spell->getLevel() != level)
			continue;

		out.push_back(spellID);
	}
}

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

bool IGameCallback::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	//only server knows
	logGlobal->error("isVisitCoveredByAnotherQuery call on client side");
	return false;
}

VCMI_LIB_NAMESPACE_END
