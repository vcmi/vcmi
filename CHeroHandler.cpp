#include "stdafx.h"
#include "CHeroHandler.h"
#include "CGameInfo.h"
#include <sstream>
#include "CGameInfo.h"
#include "CGeneralTextHandler.h"

#define CGI (CGameInfo::mainObj)

CHeroHandler::~CHeroHandler()
{
	for (int j=0;j<heroes.size();j++)
	{
		if (heroes[j]->portraitSmall)
			SDL_FreeSurface(heroes[j]->portraitSmall);
		delete heroes[j];
	}
}
void CHeroHandler::loadPortraits()
{
	std::ifstream of("portrety.txt");
	for (int j=0;j<heroes.size();j++)
	{
		int ID;
		of>>ID;
		std::string path;
		of>>path;
		heroes[ID]->portraitSmall=CGI->bitmaph->loadBitmap(path);
		if (!heroes[ID]->portraitSmall)
			std::cout<<"Can't read portrait for "<<ID<<" ("<<path<<")\n";
	}
	of.close();
}
void CHeroHandler::loadHeroes()
{
	int ID=0;
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("HOTRAITS.TXT");
	int it=0;
	std::string dump;
	for(int i=0; i<2; ++i)
	{
		CGeneralTextHandler::loadToIt(dump,buf,it,3);
	}

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

	
	for (int i=0; i<HEROES_QUANTITY; i++)
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

		std::string pom ;
		CGeneralTextHandler::loadToIt(nher->name,buf,it,4);

		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nher->low1stack = atoi(pom.c_str());
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nher->high1stack = atoi(pom.c_str());
		CGeneralTextHandler::loadToIt(nher->refType1stack,buf,it,4);

		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nher->low2stack = atoi(pom.c_str());
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nher->high2stack = atoi(pom.c_str());
		CGeneralTextHandler::loadToIt(nher->refType2stack,buf,it,4);

		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nher->low3stack = atoi(pom.c_str());
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nher->high3stack = atoi(pom.c_str());
		CGeneralTextHandler::loadToIt(nher->refType3stack,buf,it,3);
	
		nher->ID = heroes.size();
		heroes.push_back(nher);
	}
	loadSpecialAbilities();
	loadBiographies();
	loadHeroClasses();
	initHeroClasses();
	return;

}
void CHeroHandler::loadSpecialAbilities()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("HEROSPEC.TXT");
	int it=0;
	std::string dump;
	for(int i=0; i<2; ++i)
	{
		CGeneralTextHandler::loadToIt(dump,buf,it,3);
	}
	for (int i=0;i<heroes.size();i++)
	{
		CGeneralTextHandler::loadToIt(heroes[i]->bonusName,buf,it,4);
		CGeneralTextHandler::loadToIt(heroes[i]->shortBonus,buf,it,4);
		CGeneralTextHandler::loadToIt(heroes[i]->longBonus,buf,it,3);
	}
}

void CHeroHandler::loadBiographies()
{	
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("HEROBIOS.TXT");
	int it=0;
	for (int i=0;i<heroes.size();i++)
	{
		CGeneralTextHandler::loadToIt(heroes[i]->biography,buf,it,3);
	}
}

void CHeroHandler::loadHeroClasses()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("HCTRAITS.TXT");
	int andame = buf.size();
	for(int y=0; y<andame; ++y)
		if(buf[y]==',')
			buf[y]='.';
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
