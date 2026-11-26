/*
 * ZoneFactionResolver.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ZoneFactionResolver.h"

#include "CMapGenOptions.h"
#include "Functions.h"
#include "RmgMap.h"
#include "Zone.h"
#include "../GameLibrary.h"
#include "../TerrainHandler.h"
#include "../entities/faction/CFaction.h"
#include "../entities/faction/CTownHandler.h"

#include <vstd/RNG.h>

ZoneFactionResolver::ZoneFactionResolver(RmgMap & map)
	: map(map)
{
}

bool ZoneFactionResolver::hasHint(const Zone & zone, size_t hintIndex) const
{
	const auto & hints = zone.getTownHints();
	if(hints.size() <= hintIndex)
		return false;

	const auto & townHints = hints[hintIndex];
	return townHints.likeZone != rmg::ZoneOptions::NO_ZONE
		|| !townHints.notLikeZone.empty()
		|| townHints.relatedToZoneTerrain != rmg::ZoneOptions::NO_ZONE;
}

FactionID ZoneFactionResolver::fromHint(Zone & zone, size_t hintIndex)
{
	const auto & hints = zone.getTownHints();
	if(hints.size() <= hintIndex)
	{
		return *RandomGeneratorUtil::nextItem(zone.getTownTypes(), zone.getRand());
	}

	const auto & townHints = hints[hintIndex];
	FactionID subType = zone.getTownType();

	if(townHints.likeZone != rmg::ZoneOptions::NO_ZONE)
	{
		// Copy directly from other zone
		subType = map.getZones().at(townHints.likeZone)->getTownType();
	}
	else if(!townHints.notLikeZone.empty())
	{
		// Exclude type rolled for other zone
		auto townTypes = zone.getTownTypes();
		for(auto zoneId : townHints.notLikeZone)
		{
			townTypes.erase(map.getZones().at(zoneId)->getTownType());
		}
		zone.setTownTypes(townTypes);

		if(!townTypes.empty())
			subType = *RandomGeneratorUtil::nextItem(townTypes, zone.getRand());
	}
	else if(townHints.relatedToZoneTerrain != rmg::ZoneOptions::NO_ZONE)
	{
		auto townTerrain = map.getZones().at(townHints.relatedToZoneTerrain)->getTerrainType();

		auto townTypesAllowed = zone.getTownTypes();
		vstd::erase_if(townTypesAllowed, [townTerrain](FactionID type)
		{
			return !(*LIBRARY->townh)[type]->isNativeTerrain(townTerrain);
		});
		zone.setTownTypes(townTypesAllowed);

		if(!townTypesAllowed.empty())
			subType = *RandomGeneratorUtil::nextItem(townTypesAllowed, zone.getRand());
	}

	return subType;
}

FactionID ZoneFactionResolver::randomTownType(Zone & zone, bool matchUndergroundType)
{
	auto townTypesAllowed = (!zone.getTownTypes().empty() ? zone.getTownTypes() : zone.getDefaultTownTypes(*LIBRARY));
	if(matchUndergroundType)
	{
		std::set<FactionID> townTypesVerify;
		for(auto factionIdx : townTypesAllowed)
		{
			bool preferUnderground = (*LIBRARY->townh)[factionIdx]->preferUndergroundPlacement;
			if(zone.isUnderground() ? preferUnderground : !preferUnderground)
			{
				townTypesVerify.insert(factionIdx);
			}
		}
		if(!townTypesVerify.empty())
			townTypesAllowed = townTypesVerify;
	}

	return *RandomGeneratorUtil::nextItem(townTypesAllowed, zone.getRand());
}

FactionID ZoneFactionResolver::mainFaction(Zone & zone)
{
	auto & rand = zone.getRand();

	if(zone.getOwner() && ((zone.getType() == ETemplateZoneType::CPU_START) || (zone.getType() == ETemplateZoneType::PLAYER_START)))
	{
		int player_id = *zone.getOwner() - 1;
		const auto & playerSettings = map.getMapGenOptions().getPlayersSettings();

		if(playerSettings.size() > player_id)
		{
			const auto & currentPlayerSettings = std::next(playerSettings.begin(), player_id);
			const FactionID pick = currentPlayerSettings->second.getStartingTown();
			if(pick != FactionID::RANDOM)
				return pick;

			if(hasHint(zone, 0))
			{
				const FactionID hinted = fromHint(zone, 0);
				if(factionHasTown(hinted))
					return hinted;
			}
			return randomTownType(zone, true);
		}

		return townFactionOrRandom(fromHint(zone, 0), zone.getTownTypes(), rand);
	}

	// Outside an owned start zone only neutral towns are placed
	const bool hasTowns = zone.getNeutralTowns().getCastleCount() || zone.getNeutralTowns().getTownCount();

	if(hasTowns)
		return townFactionOrRandom(fromHint(zone, 0), zone.getTownTypes(), rand);

	//if there's no town present, get random faction for dwellings and pandoras - unless a town hint pinned it already
	if(hasHint(zone, 0))
		return fromHint(zone, 0);

	//25% chance for neutral
	if(rand.nextInt(1, 100) <= 25)
		return ETownType::NEUTRAL;

	if(!zone.getTownTypes().empty())
		return *RandomGeneratorUtil::nextItem(zone.getTownTypes(), rand);
	if(!zone.getMonsterTypes().empty())
		return *RandomGeneratorUtil::nextItem(zone.getMonsterTypes(), rand); //this happens in Clash of Dragons in treasure zones, where all towns are banned
	return randomTownType(zone); //just in any case
}

void ZoneFactionResolver::initTerrainType(Zone & zone)
{
	if(zone.getType() == ETemplateZoneType::WATER)
	{
		//collect all water terrain types
		std::vector<TerrainId> waterTerrains;
		for(const auto & terrain : LIBRARY->terrainTypeHandler->getObjects())
			if(terrain->isWater())
				waterTerrains.push_back(terrain->getId());

		zone.setTerrainType(*RandomGeneratorUtil::nextItem(waterTerrains, zone.getRand()));
		return;
	}

	if(zone.isMatchTerrainToTown() && zone.getTownType() != ETownType::NEUTRAL)
	{
		const auto & faction = (*LIBRARY->townh)[zone.getTownType()];
		const auto & nativeTerrains = faction->nativeTerrains;
		TerrainId terrainType = ETerrainId::NONE;

		// If preferred native terrain is incompatible with zone level,
		// try subsequent native terrains in declared order.
		const auto isCompatibleWithLevel = [&zone](TerrainId terrainId)
		{
			const auto & terrain = LIBRARY->terrainTypeHandler->getById(terrainId);
			return zone.isUnderground() ? terrain->isUnderground() : terrain->isSurface();
		};

		std::vector<TerrainId> compatibleTerrains;
		for(const auto & candidate : nativeTerrains)
		{
			if(isCompatibleWithLevel(candidate))
				compatibleTerrains.push_back(candidate);
		}

		// Prefer random native terrain when multiple compatible terrains exist.
		if(!compatibleTerrains.empty())
			terrainType = *RandomGeneratorUtil::nextItem(compatibleTerrains, zone.getRand());
		else if(!nativeTerrains.empty())
			terrainType = nativeTerrains.front();

		if(terrainType <= ETerrainId::NONE)
		{
			logRmg->warn("Town %s has invalid terrain type: %d", zone.getTownType(), terrainType);
			zone.setTerrainType(ETerrainId::DIRT);
		}
		else
		{
			zone.setTerrainType(terrainType);
		}
	}
	else
	{
		auto terrainTypes = zone.getTerrainTypes();
		if(terrainTypes.empty())
		{
			//Fill with all terain types by default
			for(const auto & terrain : LIBRARY->terrainTypeHandler->getObjects())
			{
				if(terrain->isLand() && terrain->isPassable())
				{
					if((terrain->isSurface() && !zone.isUnderground()) ||
						(terrain->isUnderground() && zone.isUnderground()))
					{
						terrainTypes.insert(TerrainId(terrain->getId()));
					}
				}
			}
		}
		zone.setTerrainType(*RandomGeneratorUtil::nextItem(terrainTypes, zone.getRand()));
	}

	//Now, replace disallowed terrains on surface and in the underground
	const auto & terrainType = LIBRARY->terrainTypeHandler->getById(zone.getTerrainType());

	if(zone.isUnderground())
	{
		if(!terrainType->isUnderground())
			zone.setTerrainType(ETerrainId::SUBTERRANEAN);
	}
	else
	{
		if(!terrainType->isSurface())
			zone.setTerrainType(ETerrainId::DIRT);
	}
}

void ZoneFactionResolver::run()
{
	std::map<TRmgTemplateZoneId, std::vector<rmg::ZoneOptions::CTownHints>> townHints;
	for(const auto & [zoneId, zone] : map.getZones())
		townHints[zoneId] = zone->getTownHints();

	const auto order = orderZonesByFirstTownHint(townHints);
	for(const auto zoneId : order)
	{
		auto & zone = *map.getZones().at(zoneId);
		// Water zones place no towns and keep the neutral faction
		if(zone.getType() != ETemplateZoneType::WATER)
			zone.setTownType(mainFaction(zone));
		initTerrainType(zone);
	}

	for(const auto & zoneEntry : map.getZones())
	{
		auto & zone = *zoneEntry.second;
		if(zone.areTownsSameType())
			continue;

		const auto & hints = zone.getTownHints();
		if(hints.size() <= 1)
			continue;

		std::vector<FactionID> hinted(hints.size(), zone.getTownType());
		for(size_t i = 1; i < hints.size(); ++i)
			hinted[i] = townFactionOrRandom(fromHint(zone, i), zone.getTownTypes(), zone.getRand());
		zone.setHintedTownTypes(std::move(hinted));
	}
}
