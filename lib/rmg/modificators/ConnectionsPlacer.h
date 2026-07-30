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
#include "../ConnectionReport.h"

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
	void collectNeighbourZones();
	std::pair<Zone::Lock, Zone::Lock> lockZones(std::shared_ptr<Zone> otherZone);

	/// Classifies where the initial placement grid put this zone relative to another - see ConnectionReport
	rmg::ConnectionReport::GridRelation gridRelation(const Zone & otherZone) const;

	/// Logs detailed geometry for a cross-level connection that degraded into a monolith
	void logGateFailure(const rmg::ZoneConnection & connection, const Zone & otherZone, rmg::ConnectionReport::GateFailure reason, int fullOverlapTiles, int possibleOverlapTiles) const;

protected:
	std::vector<rmg::ZoneConnection> dConnections;
	std::vector<rmg::ZoneConnection> dCompleted;
	std::map<TRmgTemplateZoneId, rmg::Tileset> dNeighbourZones;
};
