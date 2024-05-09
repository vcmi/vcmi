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

VCMI_LIB_NAMESPACE_BEGIN

class ConnectionsPlacer: public Modificator
{
public:
	MODIFICATOR(ConnectionsPlacer);
	
	void process() override;
	void init() override;
	
	void addConnection(const rmg::ZoneConnection& connection);
	
	void selfSideDirectConnection(const rmg::ZoneConnection & connection);
	void selfSideIndirectConnection(const rmg::ZoneConnection & connection);
	void otherSideConnection(const rmg::ZoneConnection & connection);
	void createBorder();

	bool shouldGenerateRoad(const rmg::ZoneConnection& connection) const;
	
protected:
	void collectNeighbourZones();
	std::pair<Zone::Lock, Zone::Lock> lockZones(std::shared_ptr<Zone> otherZone);

protected:
	std::vector<rmg::ZoneConnection> dConnections;
	std::vector<rmg::ZoneConnection> dCompleted;
	std::map<TRmgTemplateZoneId, rmg::Tileset> dNeighbourZones;
};

VCMI_LIB_NAMESPACE_END
