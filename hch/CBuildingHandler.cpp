#define VCMI_DLL
#include "../stdafx.h"
#include "CBuildingHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include <fstream>
extern CLodHandler * bitmaph;
std::string readTo(std::string &in, unsigned int &it, char end)
{
	int pom = it;
	int last = in.find_first_of(end,it);
	it+=(1+last-it);
	return in.substr(pom,last-pom);
}
unsigned int readNr(std::string &in, unsigned int &it)
{
	int last=it;
	for(;last<in.size();last++)
		if(in[last]=='\t' || in[last]=='\n' || in[last]==' ' || in[last]=='\r' || in[last]=='\n')
			break;
	if(last==in.size())
#ifndef __GNUC__
		throw new std::exception("Cannot read number...");
#else
		throw new std::exception();
#endif
	std::stringstream ss(in.substr(it,last-it));
	it+=(1+last-it);
	ss >> last;
	return last;
}
CBuilding * readBg(std::string &buf, unsigned int& it)
{
	CBuilding * nb = new CBuilding();
	nb->resources.resize(RESOURCE_QUANTITY);
	for(int res=0;res<7;res++)
		nb->resources[res] = readNr(buf,it);
	nb->refName = readTo(buf,it,'\n');
	return nb;
}
void CBuildingHandler::loadBuildings()
{
	std::string buf = bitmaph->getTextFile("BUILDING.TXT"), temp;
	unsigned int andame = buf.size(), it=0; //buf iterator

	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//read 2 lines of file info

	//read 9 special buildings for every faction
	for(int i=0;i<F_NUMBER;i++)
	{
		temp = readTo(buf,it,'\n');//read blank line and faction name
		temp = readTo(buf,it,'\n');
		for(int bg = 0; bg<9; bg++)
		{
			CBuilding *nb = readBg(buf,it);
			buildings[i][bg+17] = nb;
		}
	}

	//reading 17 neutral (common) buildings
	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//neutral buildings - skip 3 lines
	for(int bg = 0; bg<17; bg++)
	{
		CBuilding *nb = readBg(buf,it);
		for(int f=0;f<F_NUMBER;f++)
			buildings[f][bg] = new CBuilding(*nb);
		delete nb;
	}

	//reading 14 per faction dwellings
	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//dwellings - skip 2 lines
	for(int i=0;i<F_NUMBER;i++)
	{
		temp = readTo(buf,it,'\n');//read blank line
		temp = readTo(buf,it,'\n');// and faction name
		for(int bg = 0; bg<14; bg++)
		{
			CBuilding *nb = readBg(buf,it);
			buildings[i][bg+30] = nb;
		}
	}
	/////done reading BUILDING.TXT*****************************

	buf = bitmaph->getTextFile("BLDGNEUT.TXT");
	andame = buf.size(), it=0;

	for(int b=0;b<15;b++)
	{
		std::string name = readTo(buf,it,'\t'),
			description = readTo(buf,it,'\n');
		for(int fi=0;fi<F_NUMBER;fi++)
		{
			buildings[fi][b]->name = name;
			buildings[fi][b]->description = description;
		}
	}
	temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');temp = readTo(buf,it,'\n');//silo,blacksmith,moat - useless???
	//shipyard with the ship
	std::string name = readTo(buf,it,'\t'),
		description = readTo(buf,it,'\n');
	for(int fi=0;fi<F_NUMBER;fi++)
	{
		buildings[fi][20]->name = name;
		buildings[fi][20]->description = description;
	}

	for(int fi=0;fi<F_NUMBER;fi++)
	{
		buildings[fi][16]->name = readTo(buf,it,'\t'),
		buildings[fi][16]->description = readTo(buf,it,'\n');
	}
	/////done reading "BLDGNEUT.TXT"******************************

	buf = bitmaph->getTextFile("BLDGSPEC.TXT");
	andame = buf.size(), it=0;
	for(int f=0;f<F_NUMBER;f++)
	{
		for(int b=0;b<9;b++)
		{
			buildings[f][17+b]->name = readTo(buf,it,'\t');
			buildings[f][17+b]->description = readTo(buf,it,'\n');
		}
		buildings[f][26] = new CBuilding();//grail
		buildings[f][26]->name = readTo(buf,it,'\t');
		buildings[f][26]->description = readTo(buf,it,'\n');
		buildings[f][15]->name = readTo(buf,it,'\t'); //resource silo
		buildings[f][15]->description = readTo(buf,it,'\n');//resource silo
	}
	/////done reading BLDGSPEC.TXT*********************************

	buf = bitmaph->getTextFile("DWELLING.TXT");
	andame = buf.size(), it=0;
	for(int f=0;f<F_NUMBER;f++)
	{
		for(int b=0;b<14;b++)
		{
			buildings[f][30+b]->name = readTo(buf,it,'\t');
			buildings[f][30+b]->description = readTo(buf,it,'\n');
		}
	}

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
			if(!line[0])
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
