#ifndef CBUILDINGHANDLER_H
#define CBUILDINGHANDLER_H
#include "../global.h"
#include <map>
//enum EbuildingType {NEUTRAL=-1, CASTLE, RAMPART, TOWER, INFERNO, NECROPOLIS, DUNGEON, STRONGHOLD, FORTRESS, CONFLUX};
class CBuilding //a typical building encountered in every castle ;]
{
public:
	std::string name;
	std::string refName; //reference name, for identification
	int resources[7];
	std::string description;
	//EbuildingType type; //type of building (occures in many castles or is specific for one castle)
	//bool isDwelling; //true, if this building is a dwelling
};

class CBuildingHandler
{
public:
	std::map<int, std::map<int, CBuilding*> > buildings; ///< first int is the castle ID, second the building ID (in ERM-U format)
	std::map<int, std::pair<std::string,std::vector< std::vector< std::vector<int> > > > > hall; //map<castle ID, pair<hall bg name, std::vector< std::vector<building id> >[5]> - external vector is the vector of buildings in the row, internal is the list of buildings for the specific slot
	void loadBuildings(); //main loader
};

#endif //CBUILDINGHANDLER_H
