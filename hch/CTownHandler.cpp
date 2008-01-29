#include "../stdafx.h"
#include "CTownHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <sstream> 
#include "CGeneralTextHandler.h"
CTownHandler::CTownHandler()
{
	smallIcons = CGI->spriteh->giveDef("ITPA.DEF");
}
CTownHandler::~CTownHandler()
{
	delete smallIcons;
	//todo - delete structures info
}
void CTownHandler::loadNames()
{
	std::istringstream ins, names;
	ins.str(CGI->bitmaph->getTextFile("TOWNTYPE.TXT"));
	names.str(CGI->bitmaph->getTextFile("TOWNNAME.TXT"));
	int si=0;
	while (!ins.eof())
	{
		CTown town;
		ins >> town.name;
		char bufname[50];
		for (int i=0; i<NAMES_PER_TOWN; i++)
		{
			names.getline(bufname,50);
			town.names.push_back(std::string(bufname));
		}
		town.typeID=si++;
		town.bonus=towns.size();
		if (town.bonus==8) town.bonus=3; 
		if (town.name.length())
			towns.push_back(town);
	}

	std::string  strs = CGI->bitmaph->getTextFile("TCOMMAND.TXT");
	int itr=0;
	while(itr<strs.length()-1)
	{
		std::string tmp;
		CGeneralTextHandler::loadToIt(tmp, strs, itr, 3);
		tcommands.push_back(tmp);
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
	}
}
SDL_Surface * CTownHandler::getPic(int ID, bool fort, bool builded)
{
	if (ID==-1)
		return smallIcons->ourImages[0].bitmap;
	else if (ID==-2)
		return smallIcons->ourImages[1].bitmap;
	else if (ID==-3)
		return smallIcons->ourImages[2+F_NUMBER*4].bitmap;
	else if (ID>F_NUMBER || ID<-3)
		throw new std::exception("Invalid ID");
	else
	{
		int pom = 3;
		if(!fort)
			pom+=F_NUMBER*2;
		pom += ID*2;
		if (!builded)
			pom--;
		return smallIcons->ourImages[pom].bitmap;
	}
}

int CTownHandler::getTypeByDefName(std::string name)
{
	//TODO
	return 0;
}

CTownInstance::CTownInstance()
  :pos(-1,-1,-1)
{
	builded=-1;
	destroyed=-1;
	garrisonHero=NULL;
	town=NULL;
}

int CTownInstance::getSightDistance() const //TODO: finish
{
	return 10;
}
