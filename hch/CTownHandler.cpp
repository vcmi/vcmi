#define VCMI_DLL
#include "../stdafx.h"
#include "CTownHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include "../lib/VCMI_Lib.h"
extern CLodHandler * bitmaph;
void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
CTownHandler::CTownHandler()
{
	VLC->townh = this;
}
CTownHandler::~CTownHandler()
{}
void CTownHandler::loadNames()
{
	std::istringstream ins, names;
	ins.str(bitmaph->getTextFile("TOWNTYPE.TXT"));
	names.str(bitmaph->getTextFile("TOWNNAME.TXT"));
	int si=0;
	char bufname[75];
	while (!ins.eof())
	{
		CTown town;
		ins.getline(bufname,50);
		town.name = std::string(bufname);
		town.name = town.name.substr(0,town.name.size()-1);
		for (int i=0; i<NAMES_PER_TOWN; i++)
		{
			names.getline(bufname,50);
			std::string pom(bufname);
			town.names.push_back(pom.substr(0,pom.length()-1));
		}
		town.typeID=si++;
		town.bonus=towns.size();
		if (town.bonus==8) town.bonus=3;
		if (town.name.length())
			towns.push_back(town);
	}

	std::string  strs = bitmaph->getTextFile("TCOMMAND.TXT");
	int itr=0;
	while(itr<strs.length()-1)
	{
		std::string tmp;
		loadToIt(tmp, strs, itr, 3);
		tcommands.push_back(tmp);
	}

	strs = bitmaph->getTextFile("HALLINFO.TXT");
	itr=0;
	while(itr<strs.length()-1)
	{
		std::string tmp;
		loadToIt(tmp, strs, itr, 3);
		hcommands.push_back(tmp);
	}

	//read buildings coords
	std::ifstream of("config/buildings.txt");
	while(!of.eof())
	{
		Structure *vinya = new Structure;
		vinya->group = -1;
		of >> vinya->townID;
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
	of.open("config/buildings2.txt");
	int format, idt;
	std::string s;
	of >> format >> idt;
	while(!of.eof())
	{
		std::map<int,std::map<int, Structure*> >::iterator i;
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
				if((i=structures.find(castleID))!=structures.end())
					if((i2=(i->second.find(buildingID=atoi(s.c_str()))))!=(i->second.end()))
						i2->second->pos.z=itr++;
					else
						std::cout << "Warning1: No building "<<buildingID<<" in the castle "<<castleID<<std::endl;
				else
					std::cout << "Warning1: Castle "<<castleID<<" not defined."<<std::endl;
		}
	}
	of.close();
	of.clear();

	//read borders and areas names
	of.open("config/buildings3.txt");
	while(!of.eof())
	{
		std::map<int,std::map<int, Structure*> >::iterator i;
		std::map<int, Structure*>::iterator i2;
		int town, id;
		std::string border, area;
		of >> town >> id >> border >> border >> area;

		if((i=structures.find(town))!=structures.end())
			if((i2=(i->second.find(id)))!=(i->second.end()))
			{
				i2->second->borderName = border;
				i2->second->areaName = area;
			}
			else
				std::cout << "Warning2: No building "<<id<<" in the castle "<<town<<std::endl;
		else
			std::cout << "Warning2: Castle "<<town<<" not defined."<<std::endl;

	}
	of.close();
	of.clear();

	//read groups
	itr = 0;
	of.open("config/buildings4.txt");
	of >> format;
	if(format!=1)
	{
		std::cout << "Unhandled format of buildings4.txt \n";
	}
	else
	{
		of >> s;
		int itr=1;
		while(!of.eof())
		{
			std::map<int,std::map<int, Structure*> >::iterator i;
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
							if((i=structures.find(castleID))!=structures.end())
								if((i2=(i->second.find(buildingID)))!=(i->second.end()))
									i2->second->group = itr;
								else
									std::cout << "Warning3: No building "<<buildingID<<" in the castle "<<castleID<<std::endl;
							else
								std::cout << "Warning3: Castle "<<castleID<<" not defined."<<std::endl;
						}
						else //set group for selected building in ALL castles
						{
							for(i=structures.begin();i!=structures.end();i++)
							{
								for(i2=i->second.begin(); i2!=i->second.end(); i2++)
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

		for(int x=0;x<towns.size();x++)
			towns[x].basicCreatures.resize(7);

		of.open("config/basicCres.txt");
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

		of.open("config/creatures_upgr.txt");
		while(!of.eof())
		{
			int tid, lid, cid; //town,level,creature
			of >> tid >> lid >> cid;
			if(lid < towns[tid].upgradedCreatures.size())
				towns[tid].upgradedCreatures[lid]=cid;
		}
		of.close();
		of.clear();

		of.open("config/building_horde.txt");
		while(!of.eof())
		{
			int tid, lid, cid; //town,horde serial,creature level
			of >> tid >> lid >> cid;
			towns[tid].hordeLvl[--lid] = cid;
		}
		of.close();
		of.clear();

		of.open("config/mageLevel.txt");
		of >> si;
		for(itr=0; itr<si; itr++)
		{
			of >> towns[itr].mageLevel >> towns[itr].primaryRes;
		}
		of.close();
		of.clear();

		of.open("config/requirements.txt");
		while(!of.eof())
		{
			int ile, town, build, pom;
			of >> ile;
			for(int i=0;i<ile;i++)
			{
				of >> town;
				while(true)
				{
					of.getline(bufname,75);if(!(*bufname))of.getline(bufname,75);
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
}
