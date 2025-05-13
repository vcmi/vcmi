/*
 * CPrivilegedInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPrivilegedInfoCallback.h"

#include "../CPlayerState.h"
#include "../entities/artifact/EArtifactClass.h"
#include "../entities/building/CBuilding.h"
#include "../gameState/CGameState.h"
#include "../mapping/CMap.h"
#include "../spells/CSpellHandler.h"

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

VCMI_LIB_NAMESPACE_END
