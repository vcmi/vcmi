//
//  ConnectionsPlacer.hpp
//  vcmi
//
//  Created by nordsoft on 26.06.2022.
//

#pragma once
#include "Zone.h"
#include "CRmgArea.h"

class DLL_LINKAGE ConnectionsPlacer: public Modificator
{
public:
	ConnectionsPlacer(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	void process() override;
	
	void addConnection(const rmg::ZoneConnection& connection);
	
	void selfSideConnection(const rmg::ZoneConnection & connection);
	void otherSideConnection(const rmg::ZoneConnection & connection);
	
protected:
	void collectNeighbourZones();
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	std::vector<rmg::ZoneConnection> dConnections, dCompleted;
	std::map<TRmgTemplateZoneId, Rmg::Tileset> dNeighbourZones;
};
