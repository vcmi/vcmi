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

#include "RmgMap.h"
#include "Zone.h"
#include "Functions.h"
#include "CRmgTemplate.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

CRoadRandomizer::CRoadRandomizer(RmgMap & map)
	: map(map)
{
}

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

/*
Random road generation requirements:
- Every town should be connected via road
- There should be exactly one road betwen any two towns (connected MST)
	- This excludes cases when there are multiple obligatory road connections betwween two zones
- Road cannot end in a zone without town
- Wide connections should have no road
*/

void CRoadRandomizer::dropRandomRoads(vstd::RNG * rand)
{
	logGlobal->info("Starting road randomization");

	auto zones = map.getZones();
	
	// Helper lambda to set road option for all instances of a connection
	auto setRoadOptionForConnection = [&zones](int connectionId, rmg::ERoadOption roadOption)
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
	
	// Identify zones with towns
	std::set<TRmgTemplateZoneId> zonesWithTowns;
	for(const auto & zone : zones)
	{
		if(zone.second->getPlayerTowns().getTownCount() || 
		   zone.second->getPlayerTowns().getCastleCount() ||
		   zone.second->getNeutralTowns().getTownCount() ||
		   zone.second->getNeutralTowns().getCastleCount())
		{
			zonesWithTowns.insert(zone.first);
		}
	}
	
	logGlobal->info("Found %d zones with towns", zonesWithTowns.size());
	
	if(zonesWithTowns.empty())
	{
		// No towns, no roads needed - mark all RANDOM roads as FALSE
		for(auto & zonePtr : zones)
		{
			for(auto & connection : zonePtr.second->getConnections())
			{
				if(connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
				{
					setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_FALSE);
				}
			}
		}
		return;
	}
	
	// Initialize Union-Find for all zones to track connectivity
	std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> parent;
	// Track if a component (represented by its root) contains a town
	std::map<TRmgTemplateZoneId, bool> componentHasTown;

	for(const auto & zone : zones)
	{
		auto zoneId = zone.first;
		parent[zoneId] = zoneId;
		componentHasTown[zoneId] = vstd::contains(zonesWithTowns, zoneId);
	}

	// Process all connections that are already set to TRUE
	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
			{
				auto zoneA = connection.getZoneA();
				auto zoneB = connection.getZoneB();
				auto rootA = findSet(parent, zoneA);
				auto rootB = findSet(parent, zoneB);

				if(rootA != rootB)
				{
					bool hasTown = componentHasTown[rootA] || componentHasTown[rootB];
					parent[rootA] = rootB;
					componentHasTown[rootB] = hasTown;
				}
			}
		}
	}

	// Collect all RANDOM connections to be processed
	std::vector<rmg::ZoneConnection> randomRoads;
	std::map<int, bool> processedConnectionIds;

	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
			{
				// Ensure we only add each connection once
				if(processedConnectionIds.find(connection.getId()) == processedConnectionIds.end())
				{
					randomRoads.push_back(connection);
					processedConnectionIds[connection.getId()] = true;
				}
			}
		}
	}

	RandomGeneratorUtil::randomShuffle(randomRoads, *rand);

	// Process random roads using Kruskal's-like algorithm to connect components
	for(const auto& connection : randomRoads)
	{
		auto zoneA = connection.getZoneA();
		auto zoneB = connection.getZoneB();
		auto rootA = findSet(parent, zoneA);
		auto rootB = findSet(parent, zoneB);

		bool roadSet = false;

		// Only build roads if they connect different components
		if(rootA != rootB)
		{
			bool townInA = componentHasTown[rootA];
			bool townInB = componentHasTown[rootB];

			// Connect components if at least one of them contains a town.
			// This ensures we connect all town components and extend them,
			// but never connect two non-town components together.
			if(townInA || townInB)
			{
				logGlobal->info("Setting RANDOM road to TRUE for connection %d between zones %d and %d to connect town components.", connection.getId(), zoneA, zoneB);
				setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_TRUE);
				
				// Union the sets
				bool hasTown = townInA || townInB;
				parent[rootA] = rootB;
				componentHasTown[rootB] = hasTown;
				roadSet = true;
			}
		}

		if(!roadSet)
		{
			// This road was not chosen, either because it creates a cycle or connects two non-town areas
			setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_FALSE);
		}
	}

	// Trim roads ending in zones without towns (iteratively until no loose ends remain)
	bool changed = true;
	while(changed)
	{
		changed = false;
		
		for(auto & zonePtr : zones)
		{
			auto zoneId = zonePtr.first;
			
			// Skip zones with towns
			if(vstd::contains(zonesWithTowns, zoneId))
				continue;
			
			// Count ROAD_TRUE connections for this zone
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
			
			// If exactly one road enters this zone, remove it
			if(roadCount == 1)
			{
				logGlobal->info("Trimming loose end: removing road connection %d from zone %d (no town)", looseEndConnectionId, zoneId);
				setRoadOptionForConnection(looseEndConnectionId, rmg::ERoadOption::ROAD_FALSE);
				changed = true;
			}
		}
	}

	logGlobal->info("Finished road generation - created minimal spanning tree connecting all towns");
}

VCMI_LIB_NAMESPACE_END