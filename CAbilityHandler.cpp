#include "stdafx.h"
#include "CAbilityHandler.h"

void CAbilityHandler::loadAbilities()
{
	std::ifstream inp("H3bitmap.lod\\SSTRAITS.TXT", std::ios::in);
	std::string dump;
	for(int i=0; i<5; ++i)
	{
		inp>>dump;
	}
	inp.ignore();
	while(!inp.eof())
	{
		CAbility nab; //new creature, that will be read
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
		nab.name = base.substr(0, iit);
		++iit;
		iitBef=iit;

		nab.basicText = base.substr(iitBef, base.size()-iitBef);

		inp.getline(tab, 500);
		inp.getline(tab, 500);
		base = std::string(tab);

		iitBef = 0;
		iit = 0;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nab.basicText2 = base.substr(0, iit);
		++iit;
		iitBef=iit;

		nab.advText = base.substr(iitBef, base.size()-iitBef);

		inp.getline(tab, 500);
		inp.getline(tab, 500);
		base = std::string(tab);

		iitBef = 0;
		iit = 0;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nab.advText2 = base.substr(0, iit);
		++iit;
		iitBef=iit;

		nab.expText = base.substr(iitBef, base.size()-iitBef);

		inp.getline(tab, 500);
		inp.getline(tab, 500);
		base = std::string(tab);

		nab.expText2 = base;

		nab.idNumber = abilities.size();
		abilities.push_back(nab);
		delete [500] tab;
	}
}