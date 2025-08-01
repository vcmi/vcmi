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
	- This excludes cases when there are multiple road connetions betwween two zones
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
	
	// Track direct connections between zones (both TRUE and RANDOM)
	std::map<TRmgTemplateZoneId, std::map<TRmgTemplateZoneId, bool>> directConnections;
	
	// First pass: find all TRUE connections
	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			auto zoneA = connection.getZoneA();
			auto zoneB = connection.getZoneB();
			
			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
			{
				// Mark that these zones are directly connected by a TRUE road
				directConnections[zoneA][zoneB] = true;
				directConnections[zoneB][zoneA] = true;
			}
		}
	}
	
	// Track all available connections in the template (for graph connectivity analysis)
	std::map<TRmgTemplateZoneId, std::set<TRmgTemplateZoneId>> allConnections;
	std::map<std::pair<TRmgTemplateZoneId, TRmgTemplateZoneId>, int> connectionIds;
	
	// Build adjacency list for available connections and track connection IDs
	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			auto zoneA = connection.getZoneA();
			auto zoneB = connection.getZoneB();
			
			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE ||
			   connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
			{
				// Only include TRUE and RANDOM roads in connectivity analysis
				allConnections[zoneA].insert(zoneB);
				allConnections[zoneB].insert(zoneA);
				
				// Track connection ID for later use
				connectionIds[{std::min(zoneA, zoneB), std::max(zoneA, zoneB)}] = connection.getId();
			}
		}
	}
	
	// Check if there's a path between all town zones using all available connections
	// This is to verify if global connectivity is even possible
	std::map<TRmgTemplateZoneId, std::set<TRmgTemplateZoneId>> reachableTowns;
	
	for(auto startTown : zonesWithTowns)
	{
		std::queue<TRmgTemplateZoneId> q;
		std::set<TRmgTemplateZoneId> visited;
		
		q.push(startTown);
		visited.insert(startTown);
		
		while(!q.empty())
		{
			auto current = q.front();
			q.pop();
			
			for(auto neighbor : allConnections[current])
			{
				if(!vstd::contains(visited, neighbor))
				{
					visited.insert(neighbor);
					q.push(neighbor);
					
					if(vstd::contains(zonesWithTowns, neighbor))
					{
						reachableTowns[startTown].insert(neighbor);
					}
				}
			}
		}
	}
	
	// Initialize Union-Find for MST tracking
	std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> parent;
	for(auto townZone : zonesWithTowns)
	{
		parent[townZone] = townZone;
	}
	
	// Add all TRUE roads connecting town zones to MST
	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE)
			{
				auto zoneA = connection.getZoneA();
				auto zoneB = connection.getZoneB();
				
				// If both zones have towns, add to MST
				if(vstd::contains(zonesWithTowns, zoneA) && vstd::contains(zonesWithTowns, zoneB))
				{
					unionSets(parent, zoneA, zoneB);
				}
			}
		}
	}
	
	// Process all paths through true roads (BFS)
	for(auto townZone : zonesWithTowns)
	{
		std::queue<TRmgTemplateZoneId> q;
		std::set<TRmgTemplateZoneId> visited;
		
		q.push(townZone);
		visited.insert(townZone);
		
		while(!q.empty())
		{
			auto current = q.front();
			q.pop();
			
			for(auto & otherZone : directConnections[current])
			{
				if(otherZone.second && !vstd::contains(visited, otherZone.first))
				{
					visited.insert(otherZone.first);
					q.push(otherZone.first);
					
					// If this is another town, update MST
					if(vstd::contains(zonesWithTowns, otherZone.first))
					{
						unionSets(parent, townZone, otherZone.first);
					}
				}
			}
		}
	}
	
	// Process RANDOM connections 
	// First, ensure each town has at least one connection
	for(auto townZone : zonesWithTowns)
	{
		// Skip if already has a TRUE road
		if(!directConnections[townZone].empty())
			continue;
			
		// Find a random connection to upgrade to TRUE
		for(auto & zonePtr : zones)
		{
			for(auto & connection : zonePtr.second->getConnections())
			{
				if(connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM &&
				   (connection.getZoneA() == townZone || connection.getZoneB() == townZone))
				{
					auto zoneA = connection.getZoneA();
					auto zoneB = connection.getZoneB();
					
					// Don't upgrade if these zones are already directly connected by a TRUE road
					if(vstd::contains(directConnections[zoneA], zoneB) && directConnections[zoneA][zoneB])
						continue;
						
					// Upgrade to TRUE
					setRoadOptionForConnection(connection.getId(), rmg::ERoadOption::ROAD_TRUE);
					directConnections[zoneA][zoneB] = true;
					directConnections[zoneB][zoneA] = true;
					
					auto otherZone = (zoneA == townZone) ? zoneB : zoneA;
					logGlobal->info("Setting RANDOM road to TRUE for connection %d to ensure town zone %d has at least one connection (to zone %d)", 
									connection.getId(), townZone, otherZone);
					
					break;
				}
			}
			
			// Stop if we've found a connection
			if(!directConnections[townZone].empty())
				break;
		}
	}
	
	// Process remaining RANDOM roads - prioritize town connectivity
	// First collect all RANDOM roads
	std::vector<std::pair<int, std::pair<TRmgTemplateZoneId, TRmgTemplateZoneId>>> randomRoads;
	
	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			if(connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
			{
				auto id = connection.getId();
				auto zoneA = connection.getZoneA();
				auto zoneB = connection.getZoneB();
				
				// Skip if these zones are already directly connected by a TRUE road
				if(vstd::contains(directConnections[zoneA], zoneB) && directConnections[zoneA][zoneB])
				{
					setRoadOptionForConnection(id, rmg::ERoadOption::ROAD_FALSE);
					logGlobal->info("Setting RANDOM road to FALSE for connection %d - duplicate of TRUE road between zones %d and %d", 
									id, zoneA, zoneB);
					continue;
				}
				
				randomRoads.push_back(std::make_pair(id, std::make_pair(zoneA, zoneB)));
			}
		}
	}
	
	RandomGeneratorUtil::randomShuffle(randomRoads, *rand);
	
	// Process random roads - first connect town zones
	for(auto& road : randomRoads)
	{
		auto id = road.first;
		auto zoneA = road.second.first;
		auto zoneB = road.second.second;
		
		bool setToTrue = false;
		
		// If both zones have towns, check if they're already connected in the MST
		if(vstd::contains(zonesWithTowns, zoneA) && vstd::contains(zonesWithTowns, zoneB))
		{
			if(findSet(parent, zoneA) != findSet(parent, zoneB))
			{
				// Not connected, add this road to MST
				unionSets(parent, zoneA, zoneB);
				setToTrue = true;
				
				logGlobal->info("Setting RANDOM road to TRUE for connection %d between town zones %d and %d", 
								id, zoneA, zoneB);
			}
		}
		// If one zone has a town and one doesn't
		else if(vstd::contains(zonesWithTowns, zoneA) || vstd::contains(zonesWithTowns, zoneB))
		{
			TRmgTemplateZoneId townZone = vstd::contains(zonesWithTowns, zoneA) ? zoneA : zoneB;
			TRmgTemplateZoneId nonTownZone = vstd::contains(zonesWithTowns, zoneA) ? zoneB : zoneA;
			
			// Check if this town already has at least one TRUE connection
			if(directConnections[townZone].empty())
			{
				setToTrue = true;
				logGlobal->info("Setting RANDOM road to TRUE for connection %d - only connection for town zone %d", 
								id, townZone);
			}
			else
			{
				// See if this non-town zone connects to another town zone
				// This could be a potential bridge zone to connect towns
				bool connectsToOtherTown = false;
				TRmgTemplateZoneId otherTownZone = 0;
				
				for(auto connectedZone : allConnections[nonTownZone])
				{
					if(vstd::contains(zonesWithTowns, connectedZone) && connectedZone != townZone)
					{
						otherTownZone = connectedZone;
						connectsToOtherTown = true;
						break;
					}
				}
				
				if(connectsToOtherTown && findSet(parent, townZone) != findSet(parent, otherTownZone))
				{
					// This non-town zone can help connect two town zones that are not yet connected
					setToTrue = true;
					logGlobal->info("Setting RANDOM road to TRUE for connection %d - bridge through non-town zone %d to connect towns %d and %d", 
									id, nonTownZone, townZone, otherTownZone);
				}
			}
		}
		
		// Update all zones with this connection
		setRoadOptionForConnection(id, setToTrue ? rmg::ERoadOption::ROAD_TRUE : rmg::ERoadOption::ROAD_FALSE);
		
		if(setToTrue)
		{
			directConnections[zoneA][zoneB] = true;
			directConnections[zoneB][zoneA] = true;
		}
	}
	
	// Check if we have a connected graph for town zones after initial MST
	std::map<TRmgTemplateZoneId, std::set<TRmgTemplateZoneId>> connectedComponents;
	std::set<TRmgTemplateZoneId> processedTowns;
	
	for(auto townZone : zonesWithTowns)
	{
		if(vstd::contains(processedTowns, townZone))
			continue;
			
		std::set<TRmgTemplateZoneId> component;
		for(auto otherTown : zonesWithTowns)
		{
			if(findSet(parent, townZone) == findSet(parent, otherTown))
			{
				component.insert(otherTown);
				processedTowns.insert(otherTown);
			}
		}
		
		if(!component.empty())
			connectedComponents[townZone] = component;
	}
	
	// If we have more than one component, try to connect them if possible
	if(connectedComponents.size() > 1)
	{
		logGlobal->warn("Found %d disconnected town components, trying to connect them", connectedComponents.size());
		
		// Create a list of components
		std::vector<std::set<TRmgTemplateZoneId>> components;
		for(auto & component : connectedComponents)
		{
			components.push_back(component.second);
		}
		
		// For each pair of components, try to find a path between them
		for(size_t i = 0; i < components.size() - 1; i++)
		{
			bool foundBridge = false;
			
			for(size_t j = i + 1; j < components.size() && !foundBridge; j++)
			{
				// Try to find a path between any two towns in different components
				for(auto townA : components[i])
				{
					if(foundBridge) break;
					
					for(auto townB : components[j])
					{
						// Check if there's a path between townA and townB in the original template
						if(vstd::contains(reachableTowns[townA], townB))
						{
							// There's a path, now find the specific path to enable roads on
							std::queue<TRmgTemplateZoneId> q;
							std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> prev;
							
							q.push(townA);
							prev[townA] = 0; // Mark as visited with no predecessor
							
							bool found = false;
							while(!q.empty() && !found)
							{
								auto current = q.front();
								q.pop();
								
								for(auto next : allConnections[current])
								{
									if(!vstd::contains(prev, next))
									{
										prev[next] = current;
										q.push(next);
										
										if(next == townB)
										{
											found = true;
											break;
										}
									}
								}
							}
							
							// Now reconstruct the path and set all roads on this path to TRUE
							if(found)
							{
								std::vector<TRmgTemplateZoneId> path;
								TRmgTemplateZoneId current = townB;
								
								while(current != townA)
								{
									path.push_back(current);
									current = prev[current];
								}
								path.push_back(townA);
								
								// Reverse to get path from townA to townB
								std::reverse(path.begin(), path.end());
								
								logGlobal->info("Found path between town zones %d and %d, enabling all roads on this path", townA, townB);
								
								// Enable all roads on this path
								for(size_t k = 0; k < path.size() - 1; k++)
								{
									auto zoneA = path[k];
									auto zoneB = path[k+1];
									
									auto minZone = std::min(zoneA, zoneB);
									auto maxZone = std::max(zoneA, zoneB);
									
									if(vstd::contains(connectionIds, std::make_pair(minZone, maxZone)))
									{
										auto connectionId = connectionIds[std::make_pair(minZone, maxZone)];
										
										// Enable this road if it's not already TRUE
										for(auto & zonePtr : zones)
										{
											for(auto & connection : zonePtr.second->getConnections())
											{
												if(connection.getId() == connectionId && 
												   connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
												{
													setRoadOptionForConnection(connectionId, rmg::ERoadOption::ROAD_TRUE);
													directConnections[zoneA][zoneB] = true;
													directConnections[zoneB][zoneA] = true;
													
													logGlobal->info("Setting RANDOM road to TRUE for connection %d to connect components - part of path between towns %d and %d", 
														connectionId, townA, townB);
													
													break;
												}
											}
										}
									}
								}
								
								// Update Union-Find to merge components
								unionSets(parent, townA, townB);
								foundBridge = true;
								break;
							}
						}
					}
				}
			}
			
			if(!foundBridge)
			{
				logGlobal->warn("Could not find a path between component with towns [%s] and other components", 
					[&components, i]() {
						std::string result;
						for(auto town : components[i])
						{
							if(!result.empty()) result += ", ";
							result += std::to_string(town);
						}
						return result;
					}().c_str());
			}
		}
	}
	
	// Final check for connectivity between town zones
	std::set<TRmgTemplateZoneId> connectedTowns;
	if(!zonesWithTowns.empty())
	{
		auto firstTown = *zonesWithTowns.begin();
		for(auto town : zonesWithTowns)
		{
			if(findSet(parent, firstTown) == findSet(parent, town))
			{
				connectedTowns.insert(town);
			}
		}
	}
	
	logGlobal->info("Final town connectivity: %d connected out of %d total town zones", 
					connectedTowns.size(), zonesWithTowns.size());
	
	logGlobal->info("Finished road generation - created minimal spanning tree connecting all towns");
}

VCMI_LIB_NAMESPACE_END