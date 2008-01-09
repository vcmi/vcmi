#include "../stdafx.h"
#include "CHeroHandler.h"
#include "../CGameInfo.h"
#include <sstream>
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CAbilityHandler.h"
#include "../SDL_Extensions.h"
#include <cmath>
#include <iomanip>

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
	std::ifstream of("config/portrety.txt");
	for (int j=0;j<heroes.size();j++)
	{
		int ID;
		of>>ID;
		std::string path;
		of>>path;
		heroes[ID]->portraitSmall=CGI->bitmaph->loadBitmap(path);
		if (!heroes[ID]->portraitSmall)
			std::cout<<"Can't read small portrait for "<<ID<<" ("<<path<<")\n";
		path.replace(2,1,"L");
		heroes[ID]->portraitLarge=CGI->bitmaph->loadBitmap(path);
		if (!heroes[ID]->portraitLarge)
			std::cout<<"Can't read large portrait for "<<ID<<" ("<<path<<")\n";	
		SDL_SetColorKey(heroes[ID]->portraitLarge,SDL_SRCCOLORKEY,SDL_MapRGB(heroes[ID]->portraitLarge->format,0,255,255));

	}
	of.close();
	pskillsb = CGI->spriteh->giveDef("PSKILL.DEF");
	resources = CGI->spriteh->giveDef("RESOUR82.DEF");

	std::string  strs = CGI->bitmaph->getTextFile("PRISKILL.TXT");
	int itr=0;
	for (int i=0; i<PRIMARY_SKILLS; i++)
	{
		std::string tmp;
		CGeneralTextHandler::loadToIt(tmp, strs, itr, 3);
		pskillsn.push_back(tmp);
	}
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
		std::stringstream nm;
		nm<<"AH";
		nm<<std::setw(2);
		nm<<std::setfill('0');
		nm<<heroClasses.size();
		nm<<"_.DEF";
		hc->moveAnim = CGI->spriteh->giveDef(nm.str());

		for(int o=0; o<hc->moveAnim->ourImages.size(); ++o)
		{
			if(hc->moveAnim->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					hc->moveAnim->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(hc->moveAnim->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					hc->moveAnim->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(hc->moveAnim->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					hc->moveAnim->ourImages.push_back(nci);
				}
				o+=8;
			}
		}
		for(int o=0; o<hc->moveAnim->ourImages.size(); ++o)
		{
			if(hc->moveAnim->ourImages[o].groupNumber==1)
			{
				Cimage nci;
				nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o].bitmap);
				nci.groupNumber = 13;
				nci.imName = std::string();
				hc->moveAnim->ourImages.push_back(nci);
				//o+=1;
			}
			if(hc->moveAnim->ourImages[o].groupNumber==2)
			{
				Cimage nci;
				nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o].bitmap);
				nci.groupNumber = 14;
				nci.imName = std::string();
				hc->moveAnim->ourImages.push_back(nci);
				//o+=1;
			}
			if(hc->moveAnim->ourImages[o].groupNumber==3)
			{
				Cimage nci;
				nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o].bitmap);
				nci.groupNumber = 15;
				nci.imName = std::string();
				hc->moveAnim->ourImages.push_back(nci);
				//o+=1;
			}
		}

		for(int ff=0; ff<hc->moveAnim->ourImages.size(); ++ff)
		{
			CSDL_Ext::fullAlphaTransform(hc->moveAnim->ourImages[ff].bitmap);
		}
		hc->moveAnim->alphaTransformed = true;
		heroClasses.push_back(hc);
	}
}

void CHeroHandler::initHeroClasses()
{
	for(int gg=0; gg<heroes.size(); ++gg)
	{
		heroes[gg]->heroClass = heroClasses[heroes[gg]->heroType];
	}
	initTerrainCosts();
}

unsigned int CHeroInstance::getTileCost(EterrainType & ttype, Eroad & rdtype, Eriver & rvtype)
{
	unsigned int ret = type->heroClass->terrCosts[ttype];
	switch(rdtype)
	{
	case Eroad::dirtRoad:
		ret*=0.75;
		break;
	case Eroad::grazvelRoad:
		ret*=0.667;
		break;
	case Eroad::cobblestoneRoad:
		ret*=0.5;
		break;
	}
	return ret;
}

unsigned int CHeroHandler::level(unsigned int experience)
{
	if (experience==0)
		return 0; 
	else if (experience<14700) //level < 10
	{
		return (-500+20*sqrt((float)experience+1025))/(200);
	}
	else if (experience<24320) //10 - 12
	{
		if (experience>20600)
			return 12;
		else if (experience>17500)
			return 11;
		else return 10;
	}
	else //>12
	{
		int lvl=12;
		int xp = 24320; //xp needed for 13 lvl
		int td = 4464; //diff 14-13
		float mp = 1.2;
		while (experience>xp)
		{
			xp+=td;
			td*=mp;
			lvl++;
		}
		return lvl;
	}
}

unsigned int CHeroInstance::getLowestCreatureSpeed()
{
	unsigned int sl = 100;
	for(int h=0; h<army.slots.size(); ++h)
	{
		if(army.slots[h].first->speed<sl)
			sl = army.slots[h].first->speed;
	}
	return sl;
}

int3 CHeroInstance::convertPosition(int3 src, bool toh3m) //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
{
	if (toh3m)
	{
		src.x+=1;
		return src;
	}
	else
	{
		src.x-=1;
		return src;
	}
}
int3 CHeroInstance::getPosition(bool h3m) const
{
	if (h3m)
		return pos;
	else return convertPosition(pos,false);
}
void CHeroInstance::setPosition(int3 Pos, bool h3m)
{
	if (h3m)
		pos = Pos;
	else
		pos = convertPosition(Pos,true);
}
bool CHeroInstance::canWalkOnSea() const
{
	//TODO: write it - it should check if hero is flying, or something similiar
	return false;
}
int CHeroInstance::getCurrentLuck() const
{
	//TODO: write it
	return 0;
}
int CHeroInstance::getCurrentMorale() const
{
	//TODO: write it
	return 0;
}
void CHeroHandler::initTerrainCosts()
{
	std::ifstream inp;
	inp.open("config\\TERCOSTS.TXT", std::ios_base::in|std::ios_base::binary);
	int tynum;
	inp>>tynum;
	for(int i=0; i<2*tynum; i+=2)
	{
		int catNum;
		inp>>catNum;
		for(int k=0; k<catNum; ++k)
		{
			int curCost;
			inp>>curCost;
			heroClasses[i]->terrCosts.push_back(curCost);
			heroClasses[i+1]->terrCosts.push_back(curCost);
		}
	}
	inp.close();
}

int CHeroInstance::getSightDistance() const //TODO: finish
{
	return 6;
}
