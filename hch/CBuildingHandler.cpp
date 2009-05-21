#define VCMI_DLL
#include "../stdafx.h"
#include "CBuildingHandler.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include <sstream>
#include <fstream>
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

unsigned int readNr(std::string &in, int &it)
{
	int last=it;
	for(;last<in.size();last++)
		if(in[last]=='\t' || in[last]=='\n' || in[last]==' ' || in[last]=='\r' || in[last]=='\n')
			break;
	if(last==in.size())
		throw std::string("Cannot read number...");

	std::stringstream ss(in.substr(it,last-it));
	it+=(1+last-it);
	ss >> last;
	return last;
}
CBuilding * readBg(std::string &buf, int& it)
{
	CBuilding * nb = new CBuilding();
	nb->resources.resize(RESOURCE_QUANTITY);
	for(int res=0;res<7;res++)
		nb->resources[res] = readNr(buf,it);
	/*nb->refName = */readTo(buf,it,'\n');
	//reference name is ommitted, it's seems to be useless
	return nb;
}
void CBuildingHandler::loadBuildings()
{
	std::string buf = bitmaph->getTextFile("BUILDING.TXT"), temp;
	unsigned int andame = buf.size();
	int it=0; //buf iterator

	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//read 2 lines of file info

	//read 9 special buildings for every faction
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

	char line[100]; //bufor
	std::ifstream ofs("config/hall.txt");
	int castles;
	ofs>>castles;
	for(int i=0;i<castles;i++)
	{
		int tid;
		unsigned int it, box=0;
		std::string pom;
		ofs >> tid >> pom;
		hall[tid].first = pom;
		(hall[tid].second).resize(5); //rows
		for(int j=0;j<5;j++)
		{
			box = it = 0;
			ofs.getline(line,100);
			if(!line[0] || line[0] == '\n' || line[0] == '\r')
				ofs.getline(line,100);
			std::string linia(line);
			bool areboxes=true;
			while(areboxes) //read all boxes
			{
				(hall[tid].second)[j].push_back(std::vector<int>()); //push new box
				int seppos = linia.find_first_of('|',it); //position of end of this box data
				if(seppos<0)
					seppos = linia.length();
				while(it<seppos)
				{
					int last = linia.find_first_of(' ',it);
					std::stringstream ss(linia.substr(it,last-it));
					it = last + 1;
					ss >> last;
					(hall[tid].second)[j][box].push_back(last);
					areboxes = it; //wyzeruje jak nie znajdzie kolejnej spacji = koniec linii
					if(!it)
						it = seppos+1;
				}
				box++;
				it+=2;
			}
		}
	}

}

CBuildingHandler::~CBuildingHandler()
{
	for(std::map<int, std::map<int, CBuilding*> >::iterator i=buildings.begin(); i!=buildings.end(); i++)
		for(std::map<int, CBuilding*>::iterator j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;
}
const std::string & CBuilding::Name()
{
	if(name.length())
		return name;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].first;
	tlog2 << "Warning: Cannot find name text for building " << bid << "for " << tid << "town.\n";
	return "";
}

const std::string & CBuilding::Description()
{
	if(description.length())
		return description;
	else if(vstd::contains(VLC->generaltexth->buildings,tid) && vstd::contains(VLC->generaltexth->buildings[tid],bid))
		return VLC->generaltexth->buildings[tid][bid].second;
	tlog2 << "Warning: Cannot find description text for building " << bid << "for " << tid << "town.\n";
	return "";
}

CBuilding::CBuilding( int TID, int BID )
{
	tid = TID;
	bid = BID;
}