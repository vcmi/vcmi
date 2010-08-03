#ifndef __CBUILDINGHANDLER_H__
#define __CBUILDINGHANDLER_H__
#include "../global.h"
#include <map>
#include <vector>
#include <set>

/*
 * CBuildingHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//enum EbuildingType {NEUTRAL=-1, CASTLE, RAMPART, TOWER, INFERNO, NECROPOLIS, DUNGEON, STRONGHOLD, FORTRESS, CONFLUX};
class DLL_EXPORT CBuilding //a typical building encountered in every castle ;]
{
public:
	si32 tid, bid; //town ID and structure ID
	std::vector<si32> resources;
	std::string name;
	std::string description;

	const std::string &Name() const;
	const std::string &Description() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & resources & name & description;
	}
	CBuilding(int TID = -1, int BID = -1);
};

class DLL_EXPORT CBuildingHandler
{
public:
	std::map<int, std::map<int, CBuilding*> > buildings; ///< first int is the castle ID, second the building ID (in ERM-U format)
	std::map<int, std::pair<std::string,std::vector< std::vector< std::vector<int> > > > > hall; //map<castle ID, pair<hall bg name, std::vector< std::vector<building id> >[5]> - external vector is the vector of buildings in the row, internal is the list of buildings for the specific slot
	std::map<int, std::string> ERMUtoPicture[F_NUMBER]; //maps building ID to it's picture's name for each town type

	void loadBuildings(); //main loader
	~CBuildingHandler(); //d-tor
	static int campToERMU(int camp, int townType, std::set<si32> builtBuildings);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & buildings & hall & ERMUtoPicture;
	}
};


#endif // __CBUILDINGHANDLER_H__
