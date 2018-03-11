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

#include "serializer/CSerializer.h" // for SAVEGAME_MAGIC
#include "serializer/BinaryDeserializer.h"
#include "serializer/BinarySerializer.h"
#include "serializer/CLoadIntegrityValidator.h"
#include "rmg/CMapGenOptions.h"
#include "mapping/CCampaignHandler.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "StartInfo.h"
#include "CGameState.h"
#include "mapping/CMap.h"
#include "CPlayerState.h"

void CPrivilegedInfoCallback::getFreeTiles(std::vector<int3> & tiles) const
{
	std::vector<int> floors;
	for (int b = 0; b < (gs->map->twoLevel ? 2 : 1); ++b)
	{
		floors.push_back(b);
	}
	const TerrainTile *tinfo;
	for (auto zd : floors)
	{

		for (int xd = 0; xd < gs->map->width; xd++)
		{
			for (int yd = 0; yd < gs->map->height; yd++)
			{
				tinfo = getTile(int3 (xd,yd,zd));
				if (tinfo->terType != ETerrainType::WATER && tinfo->terType != ETerrainType::ROCK && !tinfo->blocked) //land and free
					tiles.push_back (int3 (xd,yd,zd));
			}
		}
	}
}

void CPrivilegedInfoCallback::getTilesInRange(std::unordered_set<int3, ShashInt3> & tiles, int3 pos, int radious, boost::optional<PlayerColor> player, int mode, int3::EDistanceFormula distanceFormula) const
{
	if(!!player && *player >= PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Illegal call to getTilesInRange!");
		return;
	}
	if (radious == -1) //reveal entire map
		getAllTiles (tiles, player, -1, 0);
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
						|| (mode == 1  && team->fogOfWarMap[xd][yd][pos.z]==0)
						|| (mode == -1 && team->fogOfWarMap[xd][yd][pos.z]==1)
					)
						tiles.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
}

void CPrivilegedInfoCallback::getAllTiles(std::unordered_set<int3, ShashInt3> & tiles, boost::optional<PlayerColor> Player, int level, int surface) const
{
	if(!!Player && *Player >= PlayerColor::PLAYER_LIMIT)
	{
		logGlobal->error("Illegal call to getAllTiles !");
		return;
	}
	bool water = surface == 0 || surface == 2,
		land = surface == 0 || surface == 1;

	std::vector<int> floors;
	if(level == -1)
	{
		for(int b = 0; b < (gs->map->twoLevel ? 2 : 1); ++b)
		{
			floors.push_back(b);
		}
	}
	else
		floors.push_back(level);

	for (auto zd : floors)
	{

		for (int xd = 0; xd < gs->map->width; xd++)
		{
			for (int yd = 0; yd < gs->map->height; yd++)
			{
				if ((getTile (int3 (xd,yd,zd))->terType == ETerrainType::WATER && water)
					|| (getTile (int3 (xd,yd,zd))->terType != ETerrainType::WATER && land))
					tiles.insert(int3(xd,yd,zd));
			}
		}
	}
}

void CPrivilegedInfoCallback::pickAllowedArtsSet(std::vector<const CArtifact *> & out, CRandomGenerator & rand)
{
	for (int j = 0; j < 3 ; j++)
		out.push_back(VLC->arth->artifacts[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_TREASURE)]);
	for (int j = 0; j < 3 ; j++)
		out.push_back(VLC->arth->artifacts[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_MINOR)]);

	out.push_back(VLC->arth->artifacts[VLC->arth->pickRandomArtifact(rand, CArtifact::ART_MAJOR)]);
}

void CPrivilegedInfoCallback::getAllowedSpells(std::vector<SpellID> & out, ui16 level)
{
	for (ui32 i = 0; i < gs->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
	{

		const CSpell *spell = SpellID(i).toSpell();
		if (isAllowed (0, spell->id) && spell->level == level)
		{
			out.push_back(spell->id);
		}
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
	StartInfo *si;

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

TerrainTile * CNonConstInfoCallback::getTile( int3 pos )
{
	if(!gs->map->isInTheMap(pos))
		return nullptr;
	return &gs->map->getTile(pos);
}

CGHeroInstance *CNonConstInfoCallback::getHero(ObjectInstanceID objid)
{
	return const_cast<CGHeroInstance*>(CGameInfoCallback::getHero(objid));
}

CGTownInstance *CNonConstInfoCallback::getTown(ObjectInstanceID objid)
{
	return const_cast<CGTownInstance*>(CGameInfoCallback::getTown(objid));
}

TeamState *CNonConstInfoCallback::getTeam(TeamID teamID)
{
	return const_cast<TeamState*>(CGameInfoCallback::getTeam(teamID));
}

TeamState *CNonConstInfoCallback::getPlayerTeam(PlayerColor color)
{
	return const_cast<TeamState*>(CGameInfoCallback::getPlayerTeam(color));
}

PlayerState * CNonConstInfoCallback::getPlayer( PlayerColor color, bool verbose )
{
	return const_cast<PlayerState*>(CGameInfoCallback::getPlayer(color, verbose));
}

CArtifactInstance * CNonConstInfoCallback::getArtInstance( ArtifactInstanceID aid )
{
	return gs->map->artInstances.at(aid.num);
}

CGObjectInstance * CNonConstInfoCallback::getObjInstance( ObjectInstanceID oid )
{
	return gs->map->objects.at(oid.num);
}

CArmedInstance * CNonConstInfoCallback::getArmyInstance(ObjectInstanceID oid)
{
	return dynamic_cast<CArmedInstance *>(getObjInstance(oid));
}

const CGObjectInstance * IGameCallback::putNewObject(Obj ID, int subID, int3 pos)
{
	NewObject no;
	no.ID = ID; //creature
	no.subID= subID;
	no.pos = pos;
	commitPackage(&no);
	return getObj(no.id); //id field will be filled during applying on gs
}

const CGCreature * IGameCallback::putNewMonster(CreatureID creID, int count, int3 pos)
{
	const CGObjectInstance *m = putNewObject(Obj::MONSTER, creID, pos);
	setObjProperty(m->id, ObjProperty::MONSTER_COUNT, count);
	setObjProperty(m->id, ObjProperty::MONSTER_POWER, (si64)1000*count);
	return dynamic_cast<const CGCreature*>(m);
}

bool IGameCallback::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	//only server knows
	assert(0);
	return false;
}
