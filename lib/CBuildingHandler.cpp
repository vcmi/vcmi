#include "StdInc.h"
#include "CBuildingHandler.h"

#include "CGeneralTextHandler.h"
#include "VCMI_Lib.h"
#include "Filesystem/CResourceLoader.h"
#include "JsonNode.h"
#include "GameConstants.h"

/*
 * CBuildingHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CBuilding * readBuilding(CLegacyConfigParser & parser, int townID, int buildID)
{
	CBuilding * ret = new CBuilding;
	ret->tid = townID;
	ret->bid = buildID;
	for (size_t i=0; i< ret->resources.size(); i++)
		ret->resources[i] = parser.readNumber();

	parser.endLine();
	return ret;
}

void CBuildingHandler::loadBuildings()
{
	CLegacyConfigParser parser("DATA/BUILDING.TXT");
	buildings.resize(GameConstants::F_NUMBER);

	parser.endLine(); // header
	parser.endLine();

	//Unique buildings
	for (size_t town=0; town<GameConstants::F_NUMBER; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		int buildID = 17;
		do
		{
			buildings[town][buildID] = readBuilding(parser, town, buildID);
			buildID++;
		}
		while (!parser.isNextEntryEmpty());
	}

	// Common buildings
	parser.endLine(); // header
	parser.endLine();
	parser.endLine();

	int buildID = 0;
	do
	{
		buildings[0][buildID] = readBuilding(parser, 0, buildID);

		for (size_t town=1; town<GameConstants::F_NUMBER; town++)
		{
			buildings[town][buildID] = new CBuilding(*buildings[0][buildID]);
			buildings[town][buildID]->tid = town;
		}
		buildID++;
	}
	while (!parser.isNextEntryEmpty());

	parser.endLine(); //header
	parser.endLine();

	//Dwellings
	for (size_t town=0; town<GameConstants::F_NUMBER; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		int buildID = 30;
		do
		{
			buildings[town][buildID] = readBuilding(parser, town, buildID);
			buildID++;
		}
		while (!parser.isNextEntryEmpty());
	}

	// Grail. It may not have entries in building.txt
	for (size_t town=0; town<GameConstants::F_NUMBER; town++)
	{
		if (!vstd::contains(buildings[town], 26))
		{
			buildings[town][26] = new CBuilding();
			buildings[town][26]->tid = town;
			buildings[town][26]->bid = 26;
		}
	}

	/////done reading BUILDING.TXT*****************************
	const JsonNode config(ResourceID("config/hall.json"));

	BOOST_FOREACH(const JsonNode &town, config["town"].Vector())
	{
		int tid = town["id"].Float();

		hall[tid].first = town["image"].String();
		(hall[tid].second).resize(5); //rows

		int row_num = 0;
		BOOST_FOREACH(const JsonNode &row, town["boxes"].Vector())
		{
			BOOST_FOREACH(const JsonNode &box, row.Vector())
			{
				(hall[tid].second)[row_num].push_back(std::vector<int>()); //push new box
				std::vector<int> &box_vec = (hall[tid].second)[row_num].back();

				BOOST_FOREACH(const JsonNode &value, box.Vector())
				{
					box_vec.push_back(value.Float());
				}
			}
			row_num ++;
		}
		assert (row_num == 5);
	}

	// Buildings dependencies. Which building depend on which other building.
	const JsonNode buildingsConf(ResourceID("config/buildings.json"));

	// Iterate for each city type
	int townID = 0;
	BOOST_FOREACH(const JsonNode &town_node, buildingsConf["town_type"].Vector())
	{
		BOOST_FOREACH(const JsonNode &node, town_node["building_requirements"].Vector())
		{
			int id = node["id"].Float();
			CBuilding * build = buildings[townID][id];
			if (build)
			{
				BOOST_FOREACH(const JsonNode &building, node["requires"].Vector())
				{
					build->requirements.insert(building.Float());
				}
			}
		}
		townID++;
	}
}

CBuildingHandler::~CBuildingHandler()
{
	for(std::vector< bmap<int, ConstTransitivePtr<CBuilding> > >::iterator i=buildings.begin(); i!=buildings.end(); i++)
		for(std::map<int, ConstTransitivePtr<CBuilding> >::iterator j=i->begin(); j!=i->end(); j++)
			j->second.dellNull();
}

static std::string emptyStr = "";

const std::string & CBuilding::Name() const
{
	if(name.length())
		return name;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].first;
	tlog2 << "Warning: Cannot find name text for building " << bid << "for " << tid << "town.\n";
	return emptyStr;
}

const std::string & CBuilding::Description() const
{
	if(description.length())
		return description;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].second;
	tlog2 << "Warning: Cannot find description text for building " << bid << "for " << tid << "town.\n";
	return emptyStr;
}

CBuilding::CBuilding( int TID, int BID )
{
	tid = TID;
	bid = BID;
}

int CBuildingHandler::campToERMU( int camp, int townType, std::set<si32> builtBuildings )
{
	using namespace boost::assign;
	static const std::vector<int> campToERMU = list_of(11)(12)(13)(7)(8)(9)(5)(16)(14)(15)(-1)(0)(1)(2)(3)(4)
		(6)(26)(17)(21)(22)(23)
		; //creature generators with banks - handled separately
	if (camp < campToERMU.size())
	{
		return campToERMU[camp];
	}

	static const std::vector<int> hordeLvlsPerTType[GameConstants::F_NUMBER] = {list_of(2), list_of(1), list_of(1)(4), list_of(0)(2),
		list_of(0), list_of(0), list_of(0), list_of(0), list_of(0)};

	int curPos = campToERMU.size();
	for (int i=0; i<7; ++i)
	{
		if(camp == curPos) //non-upgraded
			return 30 + i;
		curPos++;
		if(camp == curPos) //upgraded
			return 37 + i;
		curPos++;
		//horde building
		if (vstd::contains(hordeLvlsPerTType[townType], i))
		{
			if (camp == curPos)
			{
				if (hordeLvlsPerTType[townType][0] == i)
				{
					if(vstd::contains(builtBuildings, 37 + hordeLvlsPerTType[townType][0])) //if upgraded dwelling is built
						return 19;
					else //upgraded dwelling not presents
						return 18;
				}
				else
				{
					if(hordeLvlsPerTType[townType].size() > 1)
					{
						if(vstd::contains(builtBuildings, 37 + hordeLvlsPerTType[townType][1])) //if upgraded dwelling is built
							return 25;
						else //upgraded dwelling not presents
							return 24;
					}
				}
			}
			curPos++;
		}

	}
	assert(0);
	return -1; //not found
}

