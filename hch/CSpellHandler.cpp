#define VCMI_DLL
#include "../stdafx.h"
#include "CSpellHandler.h"
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include <boost/algorithm/string/replace.hpp>
extern CLodHandler *bitmaph;
void CSpellHandler::loadSpells()
{
	std::string buf = bitmaph->getTextFile("SPTRAITS.TXT"), pom;
	int andame = buf.size(), i=0; //buf iterator
	for(int z=0; z<5; ++z)
		loadToIt(pom,buf,i,3);

	bool combSpells=false; //true, if we are reading combat spells
	bool creatureAbility=false; //if true, only creature can use this spell
	int ifHit = 0;
	while(i<andame)
	{
		if(spells.size()==81)
			break;
		CSpell nsp; //new currently being read spell

		loadToIt(nsp.name,buf,i,4);
		if(nsp.name == std::string(""))
		{
			if(ifHit == 0)
			{
				combSpells = true;
			}
			if(ifHit == 1)
			{
				creatureAbility = true;
			}
			for(int z=0; z<3; ++z)
				loadToIt(pom,buf,i,3);
			loadToIt(nsp.name,buf,i,4);
			++ifHit;
		}

		loadToIt(nsp.abbName,buf,i,4);
		loadToIt(nsp.level,buf,i,4);

		loadToIt(pom,buf,i,4);
		nsp.earth = pom[0]=='x' ? true : false;
		loadToIt(pom,buf,i,4);
		nsp.water = pom[0]=='x' ? true : false;
		loadToIt(pom,buf,i,4);
		nsp.fire = pom[0]=='x' ? true : false;
		loadToIt(pom,buf,i,4);
		nsp.air = pom[0]=='x' ? true : false;

		nsp.costs.resize(4);
		for (int z = 0; z < 4 ; z++)
			loadToIt(nsp.costs[z],buf,i,4);
		loadToIt(nsp.power,buf,i,4);
		nsp.powers.resize(4);
		for (int z = 0; z < 4 ; z++)
			loadToIt(nsp.powers[z],buf,i,4);

		nsp.probabilities.resize(9);
		for (int z = 0; z < 9 ; z++)
			loadToIt(nsp.probabilities[z],buf,i,4);

		nsp.AIVals.resize(4);
		for (int z = 0; z < 4 ; z++)
			loadToIt(nsp.AIVals[z],buf,i,4);

		nsp.descriptions.resize(4);
		for (int z = 0; z < 4 ; z++)
		{
			loadToIt(nsp.descriptions[z],buf,i,4);
			boost::algorithm::replace_all(nsp.descriptions[z],"\"","");
		}

		loadToIt(nsp.attributes,buf,i,3);
		nsp.id = spells.size();
		nsp.combatSpell = combSpells;
		nsp.creatureAbility = creatureAbility;
		spells.push_back(nsp);
	}
}