#include "stdafx.h"
#include "CBuildingHandler.h"

void CBuildingHandler::loadBuildings()
{
	std::ifstream inp("H3bitmap.lod\\BUILDING.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=2;
	EbuildingType currType; //current type of building
	bool currDwel = false; //true, if we are reading dwellings
	while(!inp.eof())
	{
		CBuilding nbu; //currently read building
		if(buildings.size()>200 && buf.substr(i, buf.size()-i).find('\r')==std::string::npos)
			break;

		std::string firstStr;
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		firstStr = buf.substr(befi, i-befi);
		++i;

		if(firstStr == std::string(""))
		{
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}

		if(firstStr == std::string("Castle"))
		{
			currType = CASTLE;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Rampart"))
		{
			currType = RAMPART;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Tower"))
		{
			currType = TOWER;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Inferno"))
		{
			currType = INFERNO;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Necropolis"))
		{
			currType = NECROPOLIS;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Dungeon"))
		{
			currType = DUNGEON;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Stronghold"))
		{
			currType = STRONGHOLD;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Fortress"))
		{
			currType = FORTRESS;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Conflux"))
		{
			currType = CONFLUX;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Neutral Buildings"))
		{
			currType = NEUTRAL;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else if(firstStr == std::string("Dwellings"))
		{
			currDwel = true;
			for(i; i<andame; ++i) //omitting rubbish
			{
				if(buf[i]=='\r')
					break;
			}
			i+=2;
			continue;
		}
		else
		{
			nbu.wood = atoi(firstStr.c_str());
		}

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		nbu.mercury = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		nbu.ore = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		nbu.sulfur = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		nbu.crystal = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		nbu.gems = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		nbu.gold = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r' || buf[i]=='\t')
				break;
		}
		nbu.refName = buf.substr(befi, i-befi);
		i+=2;

		nbu.type = currType;
		nbu.isDwelling = currDwel;

		if(nbu.refName[0]==' ')
			nbu.refName = nbu.refName.substr(1, nbu.name.size()-1);

		buildings.push_back(nbu);
	}
	loadNames();
}

void CBuildingHandler::loadNames()
{
	std::ifstream inp("H3bitmap.lod\\BLDGSPEC.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	for(int ii=0; ii<9; ++ii)
	{
		for(int q=0; q<11; ++q)
		{
			if (q<9) //normal building names and descriptions
			{
				int befi=i;
				for(i; i<andame; ++i)
				{
					if(buf[i]=='\t')
						break;
				}
				buildings[ii*9+q].name = buf.substr(befi, i-befi);
				++i;

				befi=i;
				for(i; i<andame; ++i)
				{
					if(buf[i]=='\r')
						break;
				}
				buildings[ii*9+q].description = buf.substr(befi, i-befi);
				i+=2;
			}
			else if (q==9) //for graal buildings
			{
				CBuilding graal;
				int befi=i;
				for(i; i<andame; ++i)
				{
					if(buf[i]=='\t')
						break;
				}
				graal.name = buf.substr(befi, i-befi);
				++i;

				befi=i;
				for(i; i<andame; ++i)
				{
					if(buf[i]=='\r')
						break;
				}
				graal.description = buf.substr(befi, i-befi);
				i+=2;

				graal.type = EbuildingType(ii+1);
				graal.wood = graal.mercury = graal.ore = graal.sulfur = graal.crystal = graal.gems = graal.gold = 0;
				graal.isDwelling = false;
				graals.push_back(graal);
			}
			else //for resource silos
			{
				CBuilding graal;
				int befi=i;
				for(i; i<andame; ++i)
				{
					if(buf[i]=='\t')
						break;
				}
				graal.name = buf.substr(befi, i-befi);
				++i;

				befi=i;
				for(i; i<andame; ++i)
				{
					if(buf[i]=='\r')
						break;
				}
				graal.description = buf.substr(befi, i-befi);
				i+=2;

				graal.type = EbuildingType(ii+1);
				graal.wood = graal.mercury = graal.ore = graal.sulfur = graal.crystal = graal.gems = graal.gold = 0;
				graal.isDwelling = false;
				resourceSilos.push_back(graal);
			}
		}
	}
}
