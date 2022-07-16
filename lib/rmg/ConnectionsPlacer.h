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
#include "Zone.h"
#include "RmgArea.h"

class ConnectionsPlacer: public Modificator
{
public:
	MODIFICATOR(ConnectionsPlacer);
	
	void process() override;
	void init() override;
	
	void addConnection(const rmg::ZoneConnection& connection);
	
	void selfSideConnection(const rmg::ZoneConnection & connection);
	void otherSideConnection(const rmg::ZoneConnection & connection);
	
protected:
	void collectNeighbourZones();
	
protected:
	std::vector<rmg::ZoneConnection> dConnections, dCompleted;
	std::map<TRmgTemplateZoneId, rmg::Tileset> dNeighbourZones;
};
