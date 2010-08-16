#define VCMI_DLL
#include "../stdafx.h"
#include "CTownHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include "../lib/VCMI_Lib.h"
#include "CGeneralTextHandler.h"

extern CLodHandler * bitmaph;
void loadToIt(std::string &dest, const std::string &src, int &iter, int mode);

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
void CTownHandler::loadNames()
{
	int si, itr;
	char bufname[75];
	for (si=0; si<F_NUMBER; si++)
	{
		CTown town;
		town.typeID=si;
		town.bonus=towns.size();
		if (town.bonus==8) town.bonus=3;
		towns.push_back(town);
	}

	loadStructures();


	std::ifstream of;
	for(int x=0;x<towns.size();x++)
		towns[x].basicCreatures.resize(7);

	of.open(DATA_DIR "/config/basicCres.txt");
	while(!of.eof())
	{
		int tid, lid, cid; //town,level,creature
		of >> tid >> lid >> cid;
		if(lid < towns[tid].basicCreatures.size())
			towns[tid].basicCreatures[lid]=cid;
	}
	of.close();
	of.clear();

	for(int x=0;x<towns.size();x++)
		towns[x].upgradedCreatures.resize(7);

	of.open(DATA_DIR "/config/creatures_upgr.txt");
	while(!of.eof())
	{
		int tid, lid, cid; //town,level,creature
		of >> tid >> lid >> cid;
		if(lid < towns[tid].upgradedCreatures.size())
			towns[tid].upgradedCreatures[lid]=cid;
	}
	of.close();
	of.clear();

	of.open(DATA_DIR "/config/building_horde.txt");
	while(!of.eof())
	{
		int tid, lid, cid; //town,horde serial,creature level
		of >> tid >> lid >> cid;
		towns[tid].hordeLvl[--lid] = cid;
	}
	of.close();
	of.clear();

	of.open(DATA_DIR "/config/mageLevel.txt");
	of >> si;
	for(itr=0; itr<si; itr++)
	{
		of >> towns[itr].mageLevel >> towns[itr].primaryRes >> towns[itr].warMachine;
	}
	of.close();
	of.clear();

	of.open(DATA_DIR "/config/requirements.txt");
	requirements.resize(F_NUMBER);
	while(!of.eof())
	{
		int ile, town, build, pom;
		of >> ile;
		for(int i=0;i<ile;i++)
		{
			of >> town;
			while(true)
			{
				of.getline(bufname,75);
				if(!bufname[0] || bufname[0] == '\n' || bufname[0] == '\r')
					of.getline(bufname,75);
				std::istringstream ifs(bufname);
				ifs >> build;
				if(build<0)
					break;
				while(!ifs.eof())
				{
					ifs >> pom;
					requirements[town][build].insert(pom);
				}
			}
		}
	}
	of.close();
	of.clear();
}

void CTownHandler::loadStructures()
{
	structures.resize(F_NUMBER);
	//read buildings coords
	std::ifstream of(DATA_DIR "/config/buildings.txt");
	while(!of.eof())
	{
		Structure *vinya = new Structure;
		vinya->group = -1;
		of >> vinya->townID;
		if (vinya->townID == -1)
			break;
		of >> vinya->ID;
		of >> vinya->defName;
		vinya->name = vinya->defName; //TODO - use normal names
		of >> vinya->pos.x;
		of >> vinya->pos.y;
		vinya->pos.z = 0;
		structures[vinya->townID][vinya->ID] = vinya;
	}
	of.close();
	of.clear();

	//read building priorities
	of.open(DATA_DIR "/config/buildings2.txt");
	int format, idt;
	std::string s;
	of >> format >> idt;
	while(!of.eof())
	{
		std::vector<std::map<int, Structure*> >::iterator i;
		std::map<int, Structure*>::iterator i2;
		int itr=1, buildingID;
		int castleID;
		of >> s;
		if (s != "CASTLE")
			break;
		of >> castleID;
		while(1)
		{
			of >> s;
			if (s == "END")
				break;
			else
				if( (i=structures.begin() + castleID) != structures.end() )
					if( (i2 = i->find( buildingID = atoi(s.c_str()) )) != (i->end()) )
						i2->second->pos.z=itr++;
					else
						tlog3 << "Warning1: No building "<<buildingID<<" in the castle "<<castleID<<std::endl;
				else
					tlog3 << "Warning1: Castle "<<castleID<<" not defined."<<std::endl;
		}
	}
	of.close();
	of.clear();

	//read borders and areas names
	of.open(DATA_DIR "/config/buildings3.txt");
	while(!of.eof())
	{
		std::vector<std::map<int, Structure*> >::iterator i;
		std::map<int, Structure*>::iterator i2;
		int town, id;
		std::string border, area;
		of >> town >> id >> border >> border >> area;

		if( (i = structures.begin() + town) != structures.end() )
			if((i2=(i->find(id)))!=(i->end()))
			{
				i2->second->borderName = border;
				i2->second->areaName = area;
			}
			else
				tlog3 << "Warning2: No building "<<id<<" in the castle "<<town<<std::endl;
		else
			tlog3 << "Warning2: Castle "<<town<<" not defined."<<std::endl;

	}
	of.close();
	of.clear();

	//read groups
	int itr = 0;
	of.open(DATA_DIR "/config/buildings4.txt");
	of >> format;
	if(format!=1)
	{
		tlog1 << "Unhandled format of buildings4.txt \n";
	}
	else
	{
		of >> s;
		int itr=1;
		while(!of.eof())
		{
			std::vector<std::map<int, Structure*> >::iterator i;
			std::map<int, Structure*>::iterator i2;
			int buildingID;
			int castleID;
			itr++;
			if (s == "CASTLE")
			{
				of >> castleID;
			}
			else if(s == "ALL")
			{
				castleID = -1;
			}
			else
			{
				break;
			}
			of >> s;
			while(1) //read groups for castle
			{
				if (s == "GROUP")
				{
					while(1)
					{
						of >> s;
						if((s == "GROUP") || (s == "EOD") || (s == "CASTLE")) //
							break;
						buildingID = atoi(s.c_str());
						if(castleID>=0)
						{
							if((i = structures.begin() + castleID) != structures.end())
								if((i2=(i->find(buildingID)))!=(i->end()))
									i2->second->group = itr;
								else
									tlog3 << "Warning3: No building "<<buildingID<<" in the castle "<<castleID<<std::endl;
							else
								tlog3 << "Warning3: Castle "<<castleID<<" not defined."<<std::endl;
						}
						else //set group for selected building in ALL castles
						{
							for(i=structures.begin();i!=structures.end();i++)
							{
								for(i2=i->begin(); i2!=i->end(); i2++)
								{
									if(i2->first == buildingID)
									{
										i2->second->group = itr;
										break;
									}
								}
							}
						}
					}
					if(s == "CASTLE")
						break;
					itr++;
				}//if (s == "GROUP")
				else if(s == "EOD")
					break;
			}
		}
		of.close();
		of.clear();
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
