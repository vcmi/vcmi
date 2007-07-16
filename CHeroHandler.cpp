#include "stdafx.h"
#include "CHeroHandler.h"
#include "CGameInfo.h"
#include <sstream>
#define CGI (CGameInfo::mainObj)

void CHeroHandler::loadHeroes()
{
	int ID=0;
	std::ifstream inp("H3bitmap.lod\\HOTRAITS.TXT", std::ios::in);
	std::string dump;
	for(int i=0; i<25; ++i)
	{
		inp>>dump;
	}
	inp.ignore();
	int numberOfCurrentClassHeroes = 0;
	int currentClass = 0;
	int additHero = 0;
	EHeroClasses addTab[12];
	addTab[0] = HERO_KNIGHT;
	addTab[1] = HERO_WITCH;
	addTab[2] = HERO_KNIGHT;
	addTab[3] = HERO_WIZARD;
	addTab[4] = HERO_RANGER;
	addTab[5] = HERO_BARBARIAN;
	addTab[6] = HERO_DEATHKNIGHT;
	addTab[7] = HERO_WARLOCK;
	addTab[8] = HERO_KNIGHT;
	addTab[9] = HERO_WARLOCK;
	addTab[10] = HERO_BARBARIAN;
	addTab[11] = HERO_DEMONIAC;

	while(!inp.eof())
	{
		CHero * nher = new CHero;
		if(currentClass<18)
		{
			nher->heroType = (EHeroClasses)currentClass;
			++numberOfCurrentClassHeroes;
			if(numberOfCurrentClassHeroes==8)
			{
				numberOfCurrentClassHeroes = 0;
				++currentClass;
			}
		}
		else
		{
			nher->heroType = addTab[additHero++];
		}
		std::string base;
		char * tab = new char[500];
		int iit = 0;
		inp.getline(tab, 500);
		base = std::string(tab);
		if(base.size()<2) //ended, but some rubbish could still stay end we have something useless
		{
			loadSpecialAbilities();
			loadBiographies();
			loadHeroClasses();
			initHeroClasses();
			inp.close();
			return;
		}
		while(base[iit]!='\t')
		{
			++iit;
		}
		nher->name = base.substr(0, iit);
		++iit;
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher->low1stack = atoi(base.substr(iit, i).c_str());
				iit=i+1;
				break;
			}
		}
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher->high1stack = atoi(base.substr(iit, i).c_str());
				iit=i+1;
				break;
			}
		}
		int ipom=iit;
		while(base[ipom]!='\t')
		{
			++ipom;
		}
		nher->refType1stack = base.substr(iit, ipom-iit);
		iit=ipom+1;
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher->low2stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher->high2stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		ipom=iit;
		while(base[ipom]!='\t')
		{
			++ipom;
		}
		nher->refType2stack = base.substr(iit, ipom-iit);
		iit=ipom+1;
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher->low3stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher->high3stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		nher->refType3stack = base.substr(iit, base.size()-iit);
		nher->ID=ID++;
		heroes.push_back(nher);
		delete[500] tab;
	}
	loadSpecialAbilities();
}
void CHeroHandler::loadSpecialAbilities()
{
	std::ifstream inp("H3bitmap.lod\\HEROSPEC.txt", std::ios::in);
	std::string dump;
	for(int i=0; i<7; ++i)
	{
		inp>>dump;
	}
	inp.ignore();
	int whHero=0;
	while(!inp.eof() && whHero<heroes.size())
	{
		std::string base;
		char * tab = new char[500];
		int iitBef = 0;
		int iit = 0;
		inp.getline(tab, 500);
		base = std::string(tab);
		if(base.size()<2) //ended, but some rubbish could still stay end we have something useless
		{
			inp.close();
			return; //add counter
		}
		while(base[iit]!='\t')
		{
			++iit;
		}
		heroes[whHero]->bonusName = base.substr(0, iit);
		++iit;
		iitBef=iit;

		if(heroes[whHero]->bonusName == std::string("Ogry"))
		{
			char * tab2 = new char[500];
			inp.getline(tab2, 500);
			base += std::string(tab2);
			delete [500] tab2;
		}

		while(base[iit]!='\t')
		{
			++iit;
		}
		heroes[whHero]->shortBonus = base.substr(iitBef, iit-iitBef);
		++iit;
		iitBef=iit;

		while(base[iit]!='\t' && iit<base.size())
		{
			++iit;
		}
		heroes[whHero]->longBonus = base.substr(iitBef, iit-iitBef);
		++whHero;
		delete [500] tab;
	}
	inp.close();
}

void CHeroHandler::loadBiographies()
{
	std::ifstream inp("H3bitmap.lod\\HEROBIOS.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	inp.close();
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i = 0; //buf iterator
	for(int q=0; q<heroes.size(); ++q)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		heroes[q]->biography = buf.substr(befi, i-befi);
		i+=2;
	}
}

void CHeroHandler::loadHeroClasses()
{
	std::ifstream inp("H3bitmap.lod\\HCTRAITS.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	inp.close();
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i = 0; //buf iterator
	int hmcr = 0;
	for(i; i<andame; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;
	for(int ss=0; ss<18; ++ss) //18 classes of hero (including conflux)
	{
		CHeroClass * hc = new CHeroClass;
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->name = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->aggression = atof(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialAttack = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialDefence = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialPower = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialKnowledge = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proAttack[0] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proDefence[0] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proPower[0] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proKnowledge[0] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proAttack[1] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proDefence[1] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proPower[1] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->proKnowledge[1] = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		//CHero kkk = heroes[0];

		for(int dd=0; dd<CGameInfo::mainObj->abilh->abilities.size(); ++dd)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t')
					break;
			}
			int buff = atoi(buf.substr(befi, i-befi).c_str());
			++i;
			hc->proSec.push_back(buff);
		}

		for(int dd=0; dd<9; ++dd)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t' || buf[i]=='\r')
					break;
			}
			hc->selectionProbability[dd] = atoi(buf.substr(befi, i-befi).c_str());
			++i;
		}
		++i;
		heroClasses.push_back(hc);
	}
}

void CHeroHandler::initHeroClasses()
{
	for(int gg=0; gg<heroes.size(); ++gg)
	{
		heroes[gg]->heroClass = heroClasses[heroes[gg]->heroType];
	}
}
