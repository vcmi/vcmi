#include "stdafx.h"
#include "CCreatureHandler.h"

void CCreatureHandler::loadCreatures()
{
	std::ifstream inp("H3bitmap.lod\\ZCRTRAIT.TXT", std::ios::in|std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;

	while(i<buf.size())
	{
		if(creatures.size()>190 && buf.substr(i, buf.size()-i).find('\r')==std::string::npos)
			break;

		CCreature ncre;

		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.nameSing = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.namePl = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.wood = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.mercury = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.ore = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.sulfur = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.crystal = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.gems = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.gold = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.fightValue = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.AIValue = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.growth = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.hordeGrowth = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.hitPoints = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.speed = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.attack = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.defence = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.low1 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.high1 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.shots = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.spells = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.low2 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.high2 = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.abilityText = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		ncre.abilityRefs = buf.substr(befi, i-befi);
		i+=2;
		if(ncre.nameSing!=std::string("") && ncre.namePl!=std::string(""))
		{
			ncre.idNumber = creatures.size();
			creatures.push_back(ncre);
		}
	}
}

