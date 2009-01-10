#ifndef __CBUILDINGHANDLER_H__
#define __CBUILDINGHANDLER_H__
#include "../global.h"
#include <map>
#include <vector>
//enum EbuildingType {NEUTRAL=-1, CASTLE, RAMPART, TOWER, INFERNO, NECROPOLIS, DUNGEON, STRONGHOLD, FORTRESS, CONFLUX};
class DLL_EXPORT CBuilding //a typical building encountered in every castle ;]
{
public:
	int tid, bid; //town ID and structure ID
	std::vector<int> resources;
	std::string name;
	std::string description;

	const std::string &Name();
	const std::string &Description();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & resources & name & description;
	}
};

class DLL_EXPORT CBuildingHandler
{
public:
	std::map<int, std::map<int, CBuilding*> > buildings; ///< first int is the castle ID, second the building ID (in ERM-U format)
	std::map<int, std::pair<std::string,std::vector< std::vector< std::vector<int> > > > > hall; //map<castle ID, pair<hall bg name, std::vector< std::vector<building id> >[5]> - external vector is the vector of buildings in the row, internal is the list of buildings for the specific slot

	void loadBuildings(); //main loader
	~CBuildingHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & buildings & hall;
	}
};


#endif // __CBUILDINGHANDLER_H__
