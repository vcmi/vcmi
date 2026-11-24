/*
 * CRoadRandomizer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRoadRandomizer.h"

#include <vstd/RNG.h>

namespace
{
TRmgTemplateZoneId findSet(std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> & parent, TRmgTemplateZoneId x)
{
	if(parent[x] != x)
		parent[x] = findSet(parent, parent[x]);
	return parent[x];
}

void unionSets(std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> & parent, TRmgTemplateZoneId x, TRmgTemplateZoneId y)
{
	TRmgTemplateZoneId rx = findSet(parent, x);
	TRmgTemplateZoneId ry = findSet(parent, y);
	if(rx != ry)
		parent[rx] = ry;
}

bool isRoadCapable(const rmg::ZoneConnection & connection)
{
	return connection.getZoneA() != connection.getZoneB()
		&& connection.getConnectionType() != rmg::EConnectionType::REPULSIVE
		&& connection.getConnectionType() != rmg::EConnectionType::FICTIVE;
}
}

CRoadRandomizer::CRoadRandomizer(CRmgTemplate::Zones zones)
	: zones(std::move(zones))
{
}

/*
Random road generation:
- A connection is road-capable when it is not repulsive or fictive and joins two distinct zones.
- Non-capable connections never carry a road, including an explicit template true.
- Explicit true roads on capable connections are kept and unioned first.
- Random capable connections are shuffled and accepted when they join two components.
- Town-less leaves are then trimmed unless the remaining road is explicit.
- The result is the minimal subtree that joins all town zones in each connected component.
*/

void CRoadRandomizer::dropRandomRoads(vstd::RNG & rand)
{
	logRmg->info("Starting road randomization");

	auto setRoadOptionForConnection = [this](int connectionId, rmg::ERoadOption roadOption)
	{
		// Update all instances of this connection (A→B and B→A) to have the same road option
		for(auto & zonePtr : zones)
		{
			for(auto & connection : zonePtr.second->getConnections())
			{
				if(connection.getId() == connectionId)
				{
					zonePtr.second->setRoadOption(connectionId, roadOption);
				}
			}
		}
	};

	std::vector<rmg::ZoneConnection> uniqueConnections;
	std::set<int> seenIds;
	for(auto & zonePtr : zones)
	{
		for(const auto & connection : zonePtr.second->getConnections())
		{
			if(seenIds.insert(connection.getId()).second)
				uniqueConnections.push_back(connection);
		}
	}

	for(const auto & connection : uniqueConnections)
	{
		if(!isRoadCapable(connection))
			setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_FALSE);
	}

	std::set<TRmgTemplateZoneId> zonesWithTowns;
	for(const auto & zone : zones)
	{
		// start zones always receive a main town, even without explicit playerTowns
		bool isStartZone = zone.second->getOwner() && (zone.second->getType() == ETemplateZoneType::PLAYER_START || zone.second->getType() == ETemplateZoneType::CPU_START);

		if(isStartZone ||
		   zone.second->getPlayerTowns().getTownCount() ||
		   zone.second->getPlayerTowns().getCastleCount() ||
		   zone.second->getNeutralTowns().getTownCount() ||
		   zone.second->getNeutralTowns().getCastleCount())
		{
			zonesWithTowns.insert(zone.first);
		}
	}

	logRmg->info("Found %d zones with towns", zonesWithTowns.size());

	std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> parent;
	for(const auto & zone : zones)
		parent[zone.first] = zone.first;

	std::set<int> explicitRoads;
	std::vector<rmg::ZoneConnection> randomRoads;
	std::set<int> processedIds;

	for(auto & zonePtr : zones)
	{
		for(const auto & connection : zonePtr.second->getConnections())
		{
			if(!processedIds.insert(connection.getId()).second)
				continue;
			if(!isRoadCapable(connection))
				continue;

			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
			{
				explicitRoads.insert(connection.getId());
				unionSets(parent, connection.getZoneA(), connection.getZoneB());
			}
			else if(connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
			{
				randomRoads.push_back(connection);
			}
		}
	}

	RandomGeneratorUtil::randomShuffle(randomRoads, rand);

	for(const auto & connection : randomRoads)
	{
		auto zoneA = connection.getZoneA();
		auto zoneB = connection.getZoneB();
		if(findSet(parent, zoneA) != findSet(parent, zoneB))
		{
			logRmg->info("Setting RANDOM road to TRUE for connection %d between zones %d and %d.", connection.getId(), zoneA, zoneB);
			setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_TRUE);
			unionSets(parent, zoneA, zoneB);
		}
		else
		{
			setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_FALSE);
		}
	}

	bool changed = true;
	while(changed)
	{
		changed = false;

		for(auto & zonePtr : zones)
		{
			auto zoneId = zonePtr.first;

			if(vstd::contains(zonesWithTowns, zoneId))
				continue;

			int roadCount = 0;
			int looseEndConnectionId = -1;

			for(const auto & connection : zonePtr.second->getConnections())
			{
				if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
				{
					roadCount++;
					looseEndConnectionId = connection.getId();
				}
			}

			if(roadCount == 1 && !explicitRoads.count(looseEndConnectionId))
			{
				logRmg->info("Trimming loose end: removing road connection %d from zone %d (no town)", looseEndConnectionId, zoneId);
				setRoadOptionForConnection(looseEndConnectionId, rmg::ERoadOption::ROAD_FALSE);
				changed = true;
			}
		}
	}

	logRmg->info("Finished road generation - created minimal spanning tree connecting all towns");
}
