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
using TParentMap = std::map<TRmgTemplateZoneId, TRmgTemplateZoneId>;

TRmgTemplateZoneId findSet(TParentMap & parent, TRmgTemplateZoneId x)
{
	if(parent[x] != x)
		parent[x] = findSet(parent, parent[x]);
	return parent[x];
}

void unionSets(TParentMap & parent, TRmgTemplateZoneId x, TRmgTemplateZoneId y)
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

bool hasConnection(const rmg::ZoneOptions & zone, int connectionId)
{
	const auto & connections = zone.getConnections();
	return std::any_of(connections.begin(), connections.end(), [connectionId](const rmg::ZoneConnection & connection)
	{
		return connection.getId() == connectionId;
	});
}

// Update all instances of this connection (A→B and B→A) to have the same road option
void setRoadOptionForConnection(const CRmgTemplate::Zones & zones, int connectionId, rmg::ERoadOption roadOption)
{
	for(const auto & zonePtr : zones)
	{
		if(hasConnection(*zonePtr.second, connectionId))
			zonePtr.second->setRoadOption(connectionId, roadOption);
	}
}

// Each connection is stored by both of its zones; keep the first copy, in zone order
std::vector<rmg::ZoneConnection> collectUniqueConnections(const CRmgTemplate::Zones & zones)
{
	std::vector<rmg::ZoneConnection> uniqueConnections;
	std::set<int> seenIds;
	for(const auto & zonePtr : zones)
	{
		for(const auto & connection : zonePtr.second->getConnections())
		{
			if(seenIds.insert(connection.getId()).second)
				uniqueConnections.push_back(connection);
		}
	}
	return uniqueConnections;
}

bool hasTown(const rmg::ZoneOptions & zone)
{
	// start zones always receive a main town, even without explicit playerTowns
	bool isStartZone = zone.getOwner() && (zone.getType() == ETemplateZoneType::PLAYER_START || zone.getType() == ETemplateZoneType::CPU_START);

	return isStartZone
		|| zone.getPlayerTowns().getTownCount()
		|| zone.getPlayerTowns().getCastleCount()
		|| zone.getNeutralTowns().getTownCount()
		|| zone.getNeutralTowns().getCastleCount();
}

std::set<TRmgTemplateZoneId> findZonesWithTowns(const CRmgTemplate::Zones & zones)
{
	std::set<TRmgTemplateZoneId> zonesWithTowns;
	for(const auto & zone : zones)
	{
		if(hasTown(*zone.second))
			zonesWithTowns.insert(zone.first);
	}
	return zonesWithTowns;
}

// Returns the id of the only road of a zone, or -1 if it has none or several
int findSoleRoad(const rmg::ZoneOptions & zone)
{
	int roadCount = 0;
	int soleRoadId = -1;
	for(const auto & connection : zone.getConnections())
	{
		if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
		{
			roadCount++;
			soleRoadId = connection.getId();
		}
	}
	return roadCount == 1 ? soleRoadId : -1;
}

// Removes the road of a town-less zone that is a leaf of the road network, until none is left
void trimTownlessLeaves(const CRmgTemplate::Zones & zones, const std::set<TRmgTemplateZoneId> & zonesWithTowns, const std::set<int> & explicitRoads)
{
	bool changed = true;
	while(changed)
	{
		changed = false;

		for(const auto & zonePtr : zones)
		{
			auto zoneId = zonePtr.first;
			if(vstd::contains(zonesWithTowns, zoneId))
				continue;

			int looseEndConnectionId = findSoleRoad(*zonePtr.second);
			if(looseEndConnectionId != -1 && !vstd::contains(explicitRoads, looseEndConnectionId))
			{
				logRmg->info("Trimming loose end: removing road connection %d from zone %d (no town)", looseEndConnectionId, zoneId);
				setRoadOptionForConnection(zones, looseEndConnectionId, rmg::ERoadOption::ROAD_FALSE);
				changed = true;
			}
		}
	}
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

	TParentMap parent;
	for(const auto & zone : zones)
		parent[zone.first] = zone.first;

	std::set<int> explicitRoads;
	std::vector<rmg::ZoneConnection> randomRoads;

	for(const auto & connection : collectUniqueConnections(zones))
	{
		if(!isRoadCapable(connection))
		{
			setRoadOptionForConnection(zones, connection.getId(), rmg::ERoadOption::ROAD_FALSE);
		}
		else if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
		{
			explicitRoads.insert(connection.getId());
			unionSets(parent, connection.getZoneA(), connection.getZoneB());
		}
		else if(connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
		{
			randomRoads.push_back(connection);
		}
	}

	auto zonesWithTowns = findZonesWithTowns(zones);
	logRmg->info("Found %d zones with towns", zonesWithTowns.size());

	RandomGeneratorUtil::randomShuffle(randomRoads, rand);

	for(const auto & connection : randomRoads)
	{
		auto zoneA = connection.getZoneA();
		auto zoneB = connection.getZoneB();
		if(findSet(parent, zoneA) != findSet(parent, zoneB))
		{
			logRmg->info("Setting RANDOM road to TRUE for connection %d between zones %d and %d.", connection.getId(), zoneA, zoneB);
			setRoadOptionForConnection(zones, connection.getId(), rmg::ERoadOption::ROAD_TRUE);
			unionSets(parent, zoneA, zoneB);
		}
		else
		{
			setRoadOptionForConnection(zones, connection.getId(), rmg::ERoadOption::ROAD_FALSE);
		}
	}

	trimTownlessLeaves(zones, zonesWithTowns, explicitRoads);

	logRmg->info("Finished road generation - created minimal spanning tree connecting all towns");
}
