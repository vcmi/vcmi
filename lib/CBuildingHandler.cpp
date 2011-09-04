#define VCMI_DLL
#include "../stdafx.h"
#include "CBuildingHandler.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include <sstream>
#include <fstream>
#include <assert.h>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include "../lib/JsonNode.h"

extern CLodHandler * bitmaph;

/*
 * CBuildingHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static unsigned int readNr(std::string &in, int &it)
{
	int last=it;
	for(;last<in.size();last++)
		if(in[last]=='\t' || in[last]=='\n' || in[last]==' ' || in[last]=='\r' || in[last]=='\n')
			break;
	if(last==in.size())
		throw std::string("Cannot read number...");

	std::istringstream ss(in.substr(it,last-it));
	it+=(1+last-it);
	ss >> last;
	return last;
}
static CBuilding * readBg(std::string &buf, int& it)
{
	CBuilding * nb = new CBuilding();
	for(int res=0;res<7;res++)
		nb->resources[res] = readNr(buf,it);
	/*nb->refName = */readTo(buf,it,'\n');
	//reference name is ommitted, it's seems to be useless
	return nb;
}
void CBuildingHandler::loadBuildings()
{
	std::string buf = bitmaph->getTextFile("BUILDING.TXT"), temp;
	int it=0; //buf iterator

	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//read 2 lines of file info

	//read 9 special buildings for every faction
	buildings.resize(F_NUMBER);
	for(int i=0;i<F_NUMBER;i++)
	{
		temp = readTo(buf,it,'\n');//read blank line and faction name
		temp = readTo(buf,it,'\n');
		for(int bg = 0; bg<9; bg++)
		{
			CBuilding *nb = readBg(buf,it);
			nb->tid = i;
			nb->bid = bg+17;
			buildings[i][bg+17] = nb;
		}
	}

	//reading 17 neutral (common) buildings
	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//neutral buildings - skip 3 lines
	for(int bg = 0; bg<17; bg++)
	{
		CBuilding *nb = readBg(buf,it);
		for(int f=0;f<F_NUMBER;f++)
		{
			buildings[f][bg] = new CBuilding(*nb);
			buildings[f][bg]->tid = f;
			buildings[f][bg]->bid = bg;
		}
		delete nb;
	}

	//create Grail entries
	for(int i=0; i<F_NUMBER; i++)
		buildings[i][26] = new CBuilding(i,26);

	//reading 14 per faction dwellings
	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//dwellings - skip 2 lines
	for(int i=0;i<F_NUMBER;i++)
	{
		temp = readTo(buf,it,'\n');//read blank line
		temp = readTo(buf,it,'\n');// and faction name
		for(int bg = 0; bg<14; bg++)
		{
			CBuilding *nb = readBg(buf,it);
			nb->tid = i;
			nb->bid = bg+30;
			buildings[i][bg+30] = nb;
		}
	}
	/////done reading BUILDING.TXT*****************************
	const JsonNode config(DATA_DIR "/config/hall.json");

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

	static const std::vector<int> hordeLvlsPerTType[F_NUMBER] = {list_of(2), list_of(1), list_of(1)(4), list_of(0)(2),
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

