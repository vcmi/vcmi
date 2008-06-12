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
#include "CDefHandler.h"
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
		heroes[ID]->portraitSmall=BitmapHandler::loadBitmap(path);
		if (!heroes[ID]->portraitSmall)
			std::cout<<"Can't read small portrait for "<<ID<<" ("<<path<<")\n";
		for(int ff=0; ff<path.size(); ++ff) //size letter is usually third one, but there are exceptions an it should fix the problem
		{
			if(path[ff]=='S')
			{
				path[ff]='L';
				break;
			}
		}
		heroes[ID]->portraitLarge=BitmapHandler::loadBitmap(path);
		if (!heroes[ID]->portraitLarge)
			std::cout<<"Can't read large portrait for "<<ID<<" ("<<path<<")\n";	
		SDL_SetColorKey(heroes[ID]->portraitLarge,SDL_SRCCOLORKEY,SDL_MapRGB(heroes[ID]->portraitLarge->format,0,255,255));

	}
	of.close();
	pskillsb = CDefHandler::giveDef("PSKILL.DEF");
	resources = CDefHandler::giveDef("RESOUR82.DEF");
	un44 = CDefHandler::giveDef("UN44.DEF");

	std::string  strs = CGI->bitmaph->getTextFile("PRISKILL.TXT");
	int itr=0;
	for (int i=0; i<PRIMARY_SKILLS; i++)
	{
		std::string tmp;
		CGeneralTextHandler::loadToIt(tmp, strs, itr, 3);
		pskillsn.push_back(tmp);
	}
}
void CHeroHandler::loadHeroFlags()
{
	flags1.push_back(CDefHandler::giveDef("ABF01L.DEF")); //red
	flags1.push_back(CDefHandler::giveDef("ABF01G.DEF")); //blue
	flags1.push_back(CDefHandler::giveDef("ABF01R.DEF")); //tan
	flags1.push_back(CDefHandler::giveDef("ABF01D.DEF")); //green
	flags1.push_back(CDefHandler::giveDef("ABF01B.DEF")); //orange
	flags1.push_back(CDefHandler::giveDef("ABF01P.DEF")); //purple
	flags1.push_back(CDefHandler::giveDef("ABF01W.DEF")); //teal
	flags1.push_back(CDefHandler::giveDef("ABF01K.DEF")); //pink

	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags1[q]->ourImages.size(); ++o)
		{
			if(flags1[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags1[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags1[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags1[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags1[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags1[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags1[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags1[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags1[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags1[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags1[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags1[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags1[q]->alphaTransformed = true;
	}

	flags2.push_back(CDefHandler::giveDef("ABF02L.DEF")); //red
	flags2.push_back(CDefHandler::giveDef("ABF02G.DEF")); //blue
	flags2.push_back(CDefHandler::giveDef("ABF02R.DEF")); //tan
	flags2.push_back(CDefHandler::giveDef("ABF02D.DEF")); //green
	flags2.push_back(CDefHandler::giveDef("ABF02B.DEF")); //orange
	flags2.push_back(CDefHandler::giveDef("ABF02P.DEF")); //purple
	flags2.push_back(CDefHandler::giveDef("ABF02W.DEF")); //teal
	flags2.push_back(CDefHandler::giveDef("ABF02K.DEF")); //pink

	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags2[q]->ourImages.size(); ++o)
		{
			if(flags2[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags2[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags2[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags2[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags2[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags2[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags2[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags2[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags2[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags2[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags2[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags2[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags2[q]->alphaTransformed = true;
	}

	flags3.push_back(CDefHandler::giveDef("ABF03L.DEF")); //red
	flags3.push_back(CDefHandler::giveDef("ABF03G.DEF")); //blue
	flags3.push_back(CDefHandler::giveDef("ABF03R.DEF")); //tan
	flags3.push_back(CDefHandler::giveDef("ABF03D.DEF")); //green
	flags3.push_back(CDefHandler::giveDef("ABF03B.DEF")); //orange
	flags3.push_back(CDefHandler::giveDef("ABF03P.DEF")); //purple
	flags3.push_back(CDefHandler::giveDef("ABF03W.DEF")); //teal
	flags3.push_back(CDefHandler::giveDef("ABF03K.DEF")); //pink

	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags3[q]->ourImages.size(); ++o)
		{
			if(flags3[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags3[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags3[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags3[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags3[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags3[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags3[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags3[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags3[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags3[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags3[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags3[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags3[q]->alphaTransformed = true;
	}

	flags4.push_back(CDefHandler::giveDef("AF00.DEF")); //red
	flags4.push_back(CDefHandler::giveDef("AF01.DEF")); //blue
	flags4.push_back(CDefHandler::giveDef("AF02.DEF")); //tan
	flags4.push_back(CDefHandler::giveDef("AF03.DEF")); //green
	flags4.push_back(CDefHandler::giveDef("AF04.DEF")); //orange
	flags4.push_back(CDefHandler::giveDef("AF05.DEF")); //purple
	flags4.push_back(CDefHandler::giveDef("AF06.DEF")); //teal
	flags4.push_back(CDefHandler::giveDef("AF07.DEF")); //pink


	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags4[q]->ourImages.size(); ++o)
		{
			if(flags4[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int o=0; o<flags4[q]->ourImages.size(); ++o)
		{
			if(flags4[q]->ourImages[o].groupNumber==1)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 13;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==2)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 14;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==3)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 15;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags4[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags4[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags4[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags4[q]->alphaTransformed = true;
	}
}
void CHeroHandler::loadHeroes()
{
	int ID=0;
	std::string buf = CGI->bitmaph->getTextFile("HOTRAITS.TXT");
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

		for(int x=0;x<3;x++)
		{
			CGeneralTextHandler::loadToIt(pom,buf,it,4);
			nher->lowStack[x] = atoi(pom.c_str());
			CGeneralTextHandler::loadToIt(pom,buf,it,4);
			nher->highStack[x] = atoi(pom.c_str());
			CGeneralTextHandler::loadToIt(nher->refTypeStack[x],buf,it,(x==2) ? (3) : (4));
			int hlp = nher->refTypeStack[x].find_first_of(' ',0);
			if(hlp>=0)
				nher->refTypeStack[x].replace(hlp,1,"");
		}
	
		nher->ID = heroes.size();
		heroes.push_back(nher);
	}
	loadSpecialAbilities();
	loadBiographies();
	loadHeroClasses();
	initHeroClasses();
	expPerLevel.push_back(0);
	expPerLevel.push_back(1000);
	expPerLevel.push_back(2000);
	expPerLevel.push_back(3200);
	expPerLevel.push_back(4500);
	expPerLevel.push_back(6000);
	expPerLevel.push_back(7700);
	expPerLevel.push_back(9000);
	expPerLevel.push_back(11000);
	expPerLevel.push_back(13200);
	expPerLevel.push_back(15500);
	expPerLevel.push_back(18500);
	expPerLevel.push_back(22100);
	expPerLevel.push_back(26420);
	expPerLevel.push_back(31604);
	return;

}
void CHeroHandler::loadSpecialAbilities()
{
	std::string buf = CGI->bitmaph->getTextFile("HEROSPEC.TXT");
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
	std::string buf = CGI->bitmaph->getTextFile("HEROBIOS.TXT");
	int it=0;
	for (int i=0;i<heroes.size();i++)
	{
		CGeneralTextHandler::loadToIt(heroes[i]->biography,buf,it,3);
	}
}

void CHeroHandler::loadHeroClasses()
{
	std::string buf = CGI->bitmaph->getTextFile("HCTRAITS.TXT");
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

		hc->primChance.resize(PRIMARY_SKILLS);
		for(int x=0;x<PRIMARY_SKILLS;x++)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t')
					break;
			}
			hc->primChance[x].first = atoi(buf.substr(befi, i-befi).c_str());
			++i;
		}
		for(int x=0;x<PRIMARY_SKILLS;x++)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t')
					break;
			}
			hc->primChance[x].second = atoi(buf.substr(befi, i-befi).c_str());
			++i;
		}

		//CHero kkk = heroes[0];

		for(int dd=0; dd<CGI->abilh->abilities.size(); ++dd)
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
		hc->moveAnim = CDefHandler::giveDef(nm.str());

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
			CSDL_Ext::alphaTransform(hc->moveAnim->ourImages[ff].bitmap);
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

unsigned int CHeroHandler::level(unsigned int experience)
{
	int add=0;
	while(experience>=expPerLevel[expPerLevel.size()-1])
	{
		experience/=1.2;
		add+=1;
	}
	for(int i=expPerLevel.size()-1; i>=0; --i)
	{
		if(experience>=expPerLevel[i])
			return i+add;
	}
	return -1;
}

unsigned int CHeroHandler::reqExp(unsigned int level)
{
	level-=1;
	if(level<=expPerLevel.size())
		return expPerLevel[level];
	else
	{
		unsigned int exp = expPerLevel[expPerLevel.size()-1];
		level-=expPerLevel.size();
		while(level>0)
		{
			--level;
			exp*=1.2;
		}
	}
	return -1;
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

