#ifndef CBUILDINGHANDLER_H
#define CBUILDINGHANDLER_H

#include <vector>
#include <string>

enum EbuildingType {NEUTRAL=0, CASTLE, RAMPART, TOWER, INFERNO, NECROPOLIS, DUNGEON, STRONGHOLD, FORTRESS, CONFLUX};

class CBuilding //a typical building encountered in every castle ;]
{
public:
	std::string name;
	std::string refName; //reference name, for identification
	int wood, mercury, ore, sulfur, crystal, gems, gold;
	std::string description;
	EbuildingType type; //type of building (occures in many castles or is specific for one castle)
	bool isDwelling; //true, if this building is a dwelling
};

class CBuildingHandler
{
public:
	std::vector<CBuilding> buildings; //vector of buildings
	std::vector<CBuilding> resourceSilos; //vector with resource silos only - for castle profiled descriptions
	std::vector<CBuilding> graals; //vector with graal - type buildings only - for castle profiled descriptions
	void loadBuildings();
	void loadNames();
};

#endif //CBUILDINGHANDLER_H