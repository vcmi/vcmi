/*
 * ConnectionsPlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "../Zone.h"
#include "../RmgArea.h"

class ObjectManager;

class ConnectionsPlacer: public Modificator
{
public:
	MODIFICATOR(ConnectionsPlacer);
	
	void process() override;
	void init() override;
	
	void addConnection(const rmg::ZoneConnection& connection);
	void placeMonolithConnection(const rmg::ZoneConnection& connection);
	void forcePortalConnection(const rmg::ZoneConnection & connection);
	void selfSideDirectConnection(const rmg::ZoneConnection & connection);
	void selfSideIndirectConnection(const rmg::ZoneConnection & connection);
	void otherSideConnection(const rmg::ZoneConnection & connection);
	void createBorder();

	bool shouldGenerateRoad(const rmg::ZoneConnection& connection) const;
	
protected:
	/// The two subterranean gates of one cross-level connection, with everything needed to place them.
	struct GatePair
	{
		Zone & otherZone;
		ObjectManager & manager;
		ObjectManager & managerOther;
		rmg::Object & gate1;
		rmg::Object & gate2;
		int3 zShift; //this zone's level minus the other zone's level
		bool guarded1;
		bool guarded2;
		bool allowRoad;
	};

	/// True if the gates, at their current positions, would still pair with each other in game.
	bool gatePairingOk(const GatePair & gates) const;
	/// Search for gate positions sharing a column, i.e. at identical XY on both levels.
	bool placeGatePairInColumn(const rmg::ZoneConnection & connection, GatePair & gates, const rmg::Area & commonArea);
	/// Search for gate positions at most maxGateDistance apart, for zones that never overlap in XY.
	bool placeGatePairOffColumn(const rmg::ZoneConnection & connection, GatePair & gates, int maxGateDistance);
	/// Reserve the pairing for the gates' current positions and place them for real.
	bool commitGatePair(const rmg::ZoneConnection & connection, GatePair & gates, rmg::Path & path1, rmg::Path & path2);

	void collectNeighbourZones();
	std::pair<Zone::Lock, Zone::Lock> lockZones(std::shared_ptr<Zone> otherZone);

protected:
	std::vector<rmg::ZoneConnection> dConnections;
	std::vector<rmg::ZoneConnection> dCompleted;
	std::map<TRmgTemplateZoneId, rmg::Tileset> dNeighbourZones;
};
