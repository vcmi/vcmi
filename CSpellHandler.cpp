#include "stdafx.h"
#include "CSpellHandler.h"
#include <string>

void CSpellHandler::loadSpells()
{
	std::ifstream inp("SPTRAITS.TXT", std::ios::in | std::ios::binary);
	//std::string dump;
	bool combatSeries=false; //false - now we are reading adventure spells; true - combat spells
	//for(int i=0; i<52; ++i)
	//{
	//	inp>>dump;
	//}
	//inp.ignore(37);
	char * tpom = new char[100000];
	inp.get(tpom, 100000);
	inp.get(tpom, 100000);
	//inp.get(tpom, 1000, '\r');
	//inp.get(tpom, 1000, '\r');
	inp.ignore(1);
	std::string bb = std::string(tpom);
	//delete [100000] tpom;
	std::string ppp(inp, 0, inp.tellg());
	/*for(int ii=0; ii<10000; ++ii)
	{
		char c;
		inp.get(c);
		inp.get(c);
		ppp+=c;
		inp.ignore(100, '\n');
	}*/
	
	while(!inp.eof())
	{
		CSpell nsp;
		std::string base;
		char * tab = new char[5000];
		int iit = 0;
		int iitBef = 0;
		inp.get(tab, 5000, '\r');
		base = std::string(tab);
		if(base.size()<2)
		{
			return;
		}
		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.name = base.substr(0, iit);
		++iit;
		iitBef=iit;

		if (nsp.name==std::string("Adventure Spells"))
		{
			combatSeries=false;
			inp.getline(tab, 500);
			continue;
		}
		else if (nsp.name==std::string("Combat Spells"))
		{
			combatSeries=true;
			inp.getline(tab, 500);
			continue;
		}

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.abbName = base.substr(iitBef, iit-iitBef);
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.level = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.earth = base.substr(iitBef, iit-iitBef)==std::string("x") ? true : false;
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.water = base.substr(iitBef, iit-iitBef)==std::string("x") ? true : false;
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.fire = base.substr(iitBef, iit-iitBef)==std::string("x") ? true : false;
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.air = base.substr(iitBef, iit-iitBef)==std::string("x") ? true : false;
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.costNone = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.costBas = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.costAdv = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.costExp = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.power = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.powerNone = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.powerBas = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.powerAdv = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.powerExp = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.castle = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.rampart = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.tower = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.inferno = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.necropolis = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.dungeon = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.stronghold = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.fortress = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.conflux = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.none2 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.bas2 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.adv2 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t')
		{
			++iit;
		}
		nsp.exp2 = atoi(base.substr(iitBef, iit-iitBef).c_str());
		++iit;
		iitBef=iit;

		while(base[iit]!='\t' && iit<base.size())
		{
			++iit;
		}
		nsp.noneTip = base.substr(iitBef, iit-iitBef);

		inp.getline(tab, 500);
		inp.getline(tab, 500);
		base = std::string(tab);

		nsp.noneTip += std::string("\n");
		nsp.noneTip += base;
		nsp.noneTip += std::string("\n"); 

		inp.getline(tab, 500);
		inp.getline(tab, 500);
		base = std::string(tab);

		iit = 0;
		iitBef = 0;

		while(base[iit]!='\t' && iit<base.size())
		{
			++iit;
		}
		nsp.noneTip += base.substr(0, iit);
		++iit;
		iitBef=iit;

		//////////////
		inp.get(tab, 5000, '\r');
		base = std::string(tab);
		//////////////

		nsp.combatSpell = combatSeries;

		delete [500] tab;
		spells.push_back(nsp);
	}
	inp.ignore();
}