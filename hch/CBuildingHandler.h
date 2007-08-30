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
	std::vector<CBuilding> grails; //vector with grail - type buildings only - for castle profiled descriptions
	std::vector<CBuilding> blacksmiths; //vector with names and descriptions for blacksmith (castle - dependent)
	CBuilding blacksmith; //global name and description for blacksmiths
	CBuilding moat; //description and name of moat
	CBuilding shipyard; //castle - independent name and description of shipyard
	CBuilding shipyardWithShip; //name and description for shipyard with ship
	CBuilding artMerchant; //name and description of artifact merchant
	CBuilding l1horde, l2horde, l3horde, l4horde, l5horde; //castle - independent horde names and descriptions
	CBuilding grail; //castle - independent grail description
	CBuilding resSilo; //castle - independent resource silo name and description
	void loadBuildings(); //main loader, calls loading functions below
	void loadNames(); //loads castle - specufuc names and descriptoins
	void loadNeutNames(); //loads castle independent names and descriptions
	void loadDwellingNames(); //load names for dwellgins
};

#endif //CBUILDINGHANDLER_H