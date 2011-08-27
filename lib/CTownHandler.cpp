#define VCMI_DLL
#include "../stdafx.h"
#include "CTownHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include "../lib/VCMI_Lib.h"
#include "CGeneralTextHandler.h"
#include "../lib/JsonNode.h"
#include <boost/foreach.hpp>

/*
 * CTownHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CTownHandler::CTownHandler()
{
	VLC->townh = this;
}
CTownHandler::~CTownHandler()
{
	for( std::vector<std::map<int, Structure*> >::iterator i= structures.begin(); i!=structures.end(); i++)
		for( std::map<int, Structure*>::iterator j = i->begin(); j!=i->end(); j++)
			delete j->second;
}
void CTownHandler::loadStructures()
{
	int townID;

	for (townID=0; townID<F_NUMBER; townID++)
	{
		CTown town;
		town.typeID=townID;
		town.bonus=towns.size();
		if (town.bonus==8) town.bonus=3;
		towns.push_back(town);
	}

	for(int x=0;x<towns.size();x++) {
		/* There is actually 8 basic creatures, but we ignore the 8th
		 * entry for now */
		towns[x].basicCreatures.resize(7);
		towns[x].upgradedCreatures.resize(7);
	}

	structures.resize(F_NUMBER);

	// read city properties
	const JsonNode config(DATA_DIR "/config/buildings.json");

	// Iterate for each city type
	townID = 0;
	BOOST_FOREACH(const JsonNode &town_node, config["town_type"].Vector()) {
		int level;
		std::map<int, Structure*> &town = structures[townID];

		// Read buildings coordinates for that city
		BOOST_FOREACH(const JsonNode &node, town_node["defnames"].Vector()) {
			Structure *vinya = new Structure;
			const JsonNode *value;

			vinya->group = -1;
			vinya->townID = townID;
			vinya->ID = node["id"].Float();
			vinya->defName = node["defname"].String();
			vinya->name = vinya->defName; //TODO - use normal names
			vinya->pos.x = node["x"].Float();
			vinya->pos.y = node["y"].Float();
			vinya->pos.z = 0;
			
			value = &node["border"];
			if (!value->isNull())
				vinya->borderName = value->String();

			value = &node["area"];
			if (!value->isNull())
				vinya->areaName = value->String();

			town[vinya->ID] = vinya;
		}

		// Read buildings blit order for that city
		int itr = 1;
		BOOST_FOREACH(const JsonNode &node, town_node["blit_order"].Vector()) {
			int buildingID = node.Float();

			/* Find the building and set its order. */
			std::map<int, Structure*>::iterator i2 = town.find(buildingID);
			if (i2 != (town.end()))
				i2->second->pos.z = itr++;
			else
				tlog3 << "Warning1: No building " << buildingID << " in the castle " << townID << std::endl;
		}

		// Read basic creatures belonging to that city
		level = 0;
		BOOST_FOREACH(const JsonNode &node, town_node["creatures_basic"].Vector()) {
			// This if ignores the 8th field (WoG creature)
			if (level < towns[townID].basicCreatures.size())
				towns[townID].basicCreatures[level] = node.Float();
			level ++;
		}

		// Read upgraded creatures belonging to that city
		level = 0;
		BOOST_FOREACH(const JsonNode &node, town_node["creatures_upgraded"].Vector()) {
			towns[townID].upgradedCreatures[level] = node.Float();
			level ++;
		}

		//  Horde building creature level
		level = 0;
		BOOST_FOREACH(const JsonNode &node, town_node["horde"].Vector()) {
			towns[townID].hordeLvl[level] = node.Float();
			level ++;
		}

		// Buildings dependencies. Which building depend on which other building.
		requirements.resize(F_NUMBER);
		BOOST_FOREACH(const JsonNode &node, town_node["building_requirements"].Vector()) {
			std::set<int> &requires = requirements[townID][node["id"].Float()];

			BOOST_FOREACH(const JsonNode &building, node["requires"].Vector()) {
				requires.insert(building.Float());
			}
		}

		// Misc.
		towns[townID].mageLevel = town_node["mage_guild"].Float();
		towns[townID].primaryRes  = town_node["primary_ressource"].Float();
		towns[townID].warMachine = town_node["war_machine"].Float();

		townID ++;
	}

	int group_num=0;

	// Iterate for each city
	BOOST_FOREACH(const JsonNode &town_node, config["town_groups"].Vector()) {
		townID = town_node["id"].Float();

		// Iterate for each group for that city
		BOOST_FOREACH(const JsonNode &group, town_node["groups"].Vector()) {

			group_num ++;
		
			// Iterate for each bulding value in the group
			BOOST_FOREACH(const JsonNode &value, group.Vector()) {
				int buildingID = value.Float();

				std::vector<std::map<int, Structure*> >::iterator i;
				std::map<int, Structure*>::iterator i2;

				if (townID >= 0) {
					if ((i = structures.begin() + townID) != structures.end()) {
						if ((i2=(i->find(buildingID)))!=(i->end()))
							i2->second->group = group_num;
						else
							tlog3 << "Warning3: No building "<<buildingID<<" in the castle "<<townID<<std::endl;
					} 
					else
						tlog3 << "Warning3: Castle "<<townID<<" not defined."<<std::endl;
				} else {
					// Set group for selected building in ALL castles
					for (i=structures.begin();i!=structures.end();i++) {
						for(i2=i->begin(); i2!=i->end(); i2++) {
							if(i2->first == buildingID)	{
								i2->second->group = group_num;
								break;
							}
						}
					}
				}
			}
		}
	}
}

const std::string & CTown::Name() const
{
	if(name.length())
		return name;
	else
		return VLC->generaltexth->townTypes[typeID];
}

const std::vector<std::string> & CTown::Names() const
{
	if(names.size())
		return names;
	else 
		return VLC->generaltexth->townNames[typeID];
}
