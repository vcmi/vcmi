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

#include "CHeroHandler.h" // for CHeroHandler
#include "spells/CSpellHandler.h"// for CSpell
#include "CSkillHandler.h"// for CSkill
#include "NetPacks.h"
#include "CBonusTypeHandler.h"
#include "CModHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"
#include "bonuses/CBonusSystemNode.h"
#include "bonuses/Limiters.h"
#include "bonuses/Propagators.h"
#include "bonuses/Updaters.h"

#include "serializer/CSerializer.h" // for SAVEGAME_MAGIC
#include "serializer/BinaryDeserializer.h"
#include "serializer/BinarySerializer.h"
#include "serializer/CLoadIntegrityValidator.h"
#include "rmg/CMapGenOptions.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "mapObjects/ObjectTemplate.h"
#include "campaign/CampaignState.h"
#include "StartInfo.h"
#include "gameState/CGameState.h"
#include "gameState/CGameStateCampaign.h"
#include "mapping/CMap.h"
#include "CPlayerState.h"
#include "GameSettings.h"
#include "ScriptHandler.h"
#include "RoadHandler.h"
#include "RiverHandler.h"
#include "TerrainHandler.h"

#include "serializer/Connection.h"

VCMI_LIB_NAMESPACE_BEGIN

void CPrivilegedInfoCallback::getFreeTiles(std::vector<int3> & tiles) const
{
	std::vector<int> floors;
	floors.reserve(gs->map->levels());
	for(int b = 0; b < gs->map->levels(); ++b)
	{
		floors.push_back(b);
	}
	const TerrainTile * tinfo = nullptr;
	for (auto zd : floors)
	{
		for (int xd = 0; xd < gs->map->width; xd++)
		{
			for (int yd = 0; yd < gs->map->height; yd++)
			{
				tinfo = getTile(int3 (xd,yd,zd));
				if (tinfo->terType->isLand() && tinfo->terType->isPassable() && !tinfo->blocked) //land and free
					tiles.emplace_back(xd, yd, zd);
			}
		}
	}
}

void CPrivilegedInfoCallback::getTilesInRange(std::unordered_set<int3> & tiles,
											  const int3 & pos,
											  int radious,
											  std::optional<PlayerColor> player,
											  int mode,
											  int3::EDistanceFormula distanceFormula) const
{
	if(!!player && *player >= PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Illegal call to getTilesInRange!");
		return;
	}
	if(radious == CBuilding::HEIGHT_SKYSHIP) //reveal entire map
		getAllTiles (tiles, player, -1, MapTerrainFilterMode::NONE);
	else
	{
		const TeamState * team = !player ? nullptr : gs->getPlayerTeam(*player);
		for (int xd = std::max<int>(pos.x - radious , 0); xd <= std::min<int>(pos.x + radious, gs->map->width - 1); xd++)
		{
			for (int yd = std::max<int>(pos.y - radious, 0); yd <= std::min<int>(pos.y + radious, gs->map->height - 1); yd++)
			{
				int3 tilePos(xd,yd,pos.z);
				double distance = pos.dist(tilePos, distanceFormula);

				if(distance <= radious)
				{
					if(!player
						|| (mode == 1  && (*team->fogOfWarMap)[pos.z][xd][yd] == 0)
						|| (mode == -1 && (*team->fogOfWarMap)[pos.z][xd][yd] == 1)
					)
						tiles.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
}

void CPrivilegedInfoCallback::getAllTiles(std::unordered_set<int3> & tiles, std::optional<PlayerColor> Player, int level, MapTerrainFilterMode tileFilterMode) const
{
	if(!!Player && *Player >= PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Illegal call to getAllTiles !");
		return;
	}

	std::vector<int> floors;
	if(level == -1)
	{
		for(int b = 0; b < gs->map->levels(); ++b)
		{
			floors.push_back(b);
		}
	}
	else
		floors.push_back(level);

	for(auto zd: floors)
	{
		for(int xd = 0; xd < gs->map->width; xd++)
		{
			for(int yd = 0; yd < gs->map->height; yd++)
			{
				bool isTileEligible = false;

				switch(tileFilterMode)
				{
					case MapTerrainFilterMode::NONE:
						isTileEligible = true;
						break;
					case MapTerrainFilterMode::WATER:
						isTileEligible = getTile(int3(xd, yd, zd))->terType->isWater();
						break;
					case MapTerrainFilterMode::LAND:
						isTileEligible = getTile(int3(xd, yd, zd))->terType->isLand();
						break;
					case MapTerrainFilterMode::LAND_CARTOGRAPHER:
						isTileEligible = getTile(int3(xd, yd, zd))->terType->isSurfaceCartographerCompatible();
						break;
					case MapTerrainFilterMode::UNDERGROUND_CARTOGRAPHER:
						isTileEligible = getTile(int3(xd, yd, zd))->terType->isUndergroundCartographerCompatible();
						break;
				}

				if(isTileEligible)
					tiles.insert(int3(xd, yd, zd));
			}
		}
	}
}

void CPrivilegedInfoCallback::pickAllowedArtsSet(std::vector<const CArtifact *> & out, CRandomGenerator & rand) const
{
	for (int j = 0; j < 3 ; j++)
		out.push_back(VLC->arth->objects[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_TREASURE)]);
	for (int j = 0; j < 3 ; j++)
		out.push_back(VLC->arth->objects[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_MINOR)]);

	out.push_back(VLC->arth->objects[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_MAJOR)]);
}

void CPrivilegedInfoCallback::getAllowedSpells(std::vector<SpellID> & out, std::optional<ui16> level)
{
	for (ui32 i = 0; i < gs->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
	{
		const spells::Spell * spell = SpellID(i).toSpell();

		if (!isAllowed(0, spell->getIndex()))
			continue;

		if (level.has_value() && spell->getLevel() != level)
			continue;

		out.push_back(spell->getId());
	}
}

CGameState * CPrivilegedInfoCallback::gameState()
{
	return gs;
}

template<typename Loader>
void CPrivilegedInfoCallback::loadCommonState(Loader & in)
{
	logGlobal->info("Loading lib part of game...");
	in.checkMagicBytes(SAVEGAME_MAGIC);

	CMapHeader dum;
	StartInfo * si = nullptr;

	logGlobal->info("\tReading header");
	in.serializer & dum;

	logGlobal->info("\tReading options");
	in.serializer & si;

	logGlobal->info("\tReading handlers");
	in.serializer & *VLC;

	logGlobal->info("\tReading gamestate");
	in.serializer & gs;
}

template<typename Saver>
void CPrivilegedInfoCallback::saveCommonState(Saver & out) const
{
	logGlobal->info("Saving lib part of game...");
	out.putMagicBytes(SAVEGAME_MAGIC);
	logGlobal->info("\tSaving header");
	out.serializer & static_cast<CMapHeader&>(*gs->map);
	logGlobal->info("\tSaving options");
	out.serializer & gs->scenarioOps;
	logGlobal->info("\tSaving handlers");
	out.serializer & *VLC;
	logGlobal->info("\tSaving gamestate");
	out.serializer & gs;
}

// hardly memory usage for `-gdwarf-4` flag
template DLL_LINKAGE void CPrivilegedInfoCallback::loadCommonState<CLoadIntegrityValidator>(CLoadIntegrityValidator &);
template DLL_LINKAGE void CPrivilegedInfoCallback::loadCommonState<CLoadFile>(CLoadFile &);
template DLL_LINKAGE void CPrivilegedInfoCallback::saveCommonState<CSaveFile>(CSaveFile &) const;

TerrainTile * CNonConstInfoCallback::getTile(const int3 & pos)
{
	if(!gs->map->isInTheMap(pos))
		return nullptr;
	return &gs->map->getTile(pos);
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
	return gs->map->artInstances.at(aid.num);
}

CGObjectInstance * CNonConstInfoCallback::getObjInstance(const ObjectInstanceID & oid)
{
	return gs->map->objects.at(oid.num);
}

CArmedInstance * CNonConstInfoCallback::getArmyInstance(const ObjectInstanceID & oid)
{
	return dynamic_cast<CArmedInstance *>(getObjInstance(oid));
}

bool IGameCallback::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	//only server knows
	logGlobal->error("isVisitCoveredByAnotherQuery call on client side");
	return false;
}

VCMI_LIB_NAMESPACE_END
