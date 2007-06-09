#include "stdafx.h"
#include "CCreatureHandler.h"

void CCreatureHandler::loadCreatures()
{
	std::ifstream inp("ZCRTRAIT.TXT", std::ios::in);
	std::string dump;
	for(int i=0; i<42; ++i)
	{
		inp>>dump;
	}
	inp.ignore();
	while(!inp.eof())
	{
		CCreature ncre; //new creature, that will be read
		std::string base;
		char * tab = new char[500];
		int iitBef = 0;
		int iit = 0;
		inp.getline(tab, 500);
		base = std::string(tab);
		if(base.size()<2) //ended, but some rubbish could still stay end we have something useless
		{
			return; //add counter
		}
		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.nameSing = base.substr(0, iit);
		++iit;
		iitBef=iit;

		//////// omijanie problemu z kasowaniem znaków CR - ifujê problematyczne miejsca
		if(ncre.nameSing==std::string("Wilko³ak") || ncre.nameSing==std::string("Werewolf") 
			|| ncre.nameSing==std::string("Piekielny Rumak") || ncre.nameSing==std::string("Hell Steed")
			|| ncre.nameSing==std::string("Sylwañski Centaur") || ncre.nameSing==std::string("Sylvan Centaur"))
		{
			std::string base2;
			char * tab2 = new char[500];
			inp.getline(tab2, 500);
			base2 = std::string(tab2);
			base+=base2;
		}
		///////

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.namePl = base.substr(iitBef, iit-iitBef);
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.wood = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.mercury = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.ore = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.sulfur = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.crystal = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.gems = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.gold = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.fightValue = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.AIValue = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.growth = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.hordeGrowth = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.hitPoints = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.speed = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.attack = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.defence = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.low1 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.high1 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.shots = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.spells = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.low2 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.high2 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		ncre.abilityText = base.substr(iitBef, iit-iitBef);
		++iit;
		iitBef=iit;

		while(base[iit]!='\t' && iit<base.size())
		{
			++iit;
		}
		ncre.abilityRefs = base.substr(iitBef, iit-iitBef);
		++iit;
		iitBef=iit;

		if(ncre.nameSing!=std::string("") && ncre.namePl!=std::string(""))
		{
			ncre.idNumber=creatures.size();
			creatures.push_back(ncre);
		}
		delete[500] tab;
	}
}

