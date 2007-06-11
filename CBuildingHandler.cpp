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
	loadNeutNames();
	loadDwellingNames();
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
				grails.push_back(graal);
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
	///////////////reading artifact merchant
	int befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	artMerchant.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	artMerchant.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading level1 creature horde
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	l1horde.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	l1horde.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading level2 creature horde
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	l2horde.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	l2horde.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading shipyard
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	shipyard.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	shipyard.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////omitting rubbish
	int hmcr = 0;
	for(i; i<andame; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;
	//////////////////////reading level3 creature horde
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	l3horde.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	l3horde.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading level4 creature horde
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	l4horde.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	l4horde.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading level5 creature horde
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	l5horde.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	l5horde.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading grail
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	grail.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	grail.description = buf.substr(befi, i-befi);
	i+=2;
	//////////////////////reading resource silo
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	resSilo.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	resSilo.description = buf.substr(befi, i-befi);
	i+=2;
}

void CBuildingHandler::loadNeutNames()
{
	std::ifstream inp("H3bitmap.lod\\BLDGNEUT.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	for(int q=0; q<15; ++q)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		buildings[81+q].name = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		buildings[81+q].description = buf.substr(befi, i-befi);
		i+=2;
	}
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	i+=2;
	////////////////////////////reading blacksmith
	int befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	CBuilding b1;
	b1.type = EbuildingType(0);
	b1.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	b1.description = buf.substr(befi, i-befi);
	i+=2;
	blacksmith = b1;
	//////////////////////////////reading moat
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	b1.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	b1.description = buf.substr(befi, i-befi);
	i+=2;
	moat = b1;
	/////////////////////////reading shipyard with ship
	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	b1.name = buf.substr(befi, i-befi);
	++i;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	b1.description = buf.substr(befi, i-befi);
	i+=2;
	shipyardWithShip = b1;
	/////////////////////////reading blacksmiths
	for(int q=0; q<9; ++q)
	{
		CBuilding black; //
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		black.name = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		black.description = buf.substr(befi, i-befi);
		i+=2;

		black.type = EbuildingType(q+1);
		blacksmiths.push_back(black);
	}
}

void CBuildingHandler::loadDwellingNames()
{
	std::ifstream inp("H3bitmap.lod\\DWELLING.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i = 0; //buf iterator
	int whdw = 98; //wchich dwelling we are currently reading
	for(whdw; whdw<224; ++whdw)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		buildings[whdw].name = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		buildings[whdw].description = buf.substr(befi, i-befi);
		i+=2;
	}
}
