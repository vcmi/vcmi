#include "../stdafx.h"
#include "Graphics.h"
#include "../hch/CDefHandler.h"
#include "../hch/CObjectHandler.h"
#include "../SDL_Extensions.h"
#include <boost/assign/std/vector.hpp> 
#include <sstream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/std/vector.hpp>
#include "../CThreadHelper.h"
#include "../CGameInfo.h"
#include "../hch/CLodHandler.h"
using namespace boost::assign;
using namespace CSDL_Ext;
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
Graphics * graphics = NULL;
SDL_Surface * Graphics::drawPrimarySkill(const CGHeroInstance *curh, SDL_Surface *ret, int from, int to)
{
	char * buf = new char[10];
	for (int i=from;i<to;i++)
	{
		SDL_itoa(curh->getPrimSkillLevel(i),buf,10);
		printAtMiddle(buf,84+28*i,68,GEOR13,zwykly,ret);
	}
	delete[] buf;
	return ret;
}
SDL_Surface * Graphics::drawHeroInfoWin(const CGHeroInstance * curh)
{
	char * buf = new char[10];
	blueToPlayersAdv(hInfo,curh->tempOwner);
	SDL_Surface * ret = SDL_DisplayFormat(hInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	printAt(curh->name,75,15,GEOR13,zwykly,ret);
	drawPrimarySkill(curh, ret);
	for (std::map<si32,std::pair<ui32,si32> >::const_iterator i=curh->army.slots.begin(); i!=curh->army.slots.end();i++)
	{
		blitAt(graphics->smallImgs[(*i).second.first],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		SDL_itoa((*i).second.second,buf,10);
		printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+39,GEORM,zwykly,ret);
	}
	blitAt(graphics->portraitLarge[curh->portrait],11,12,ret);
	SDL_itoa(curh->mana,buf,10);
	printAtMiddle(buf,166,109,GEORM,zwykly,ret); //mana points
	delete[] buf;
	blitAt(morale22->ourImages[curh->getCurrentMorale()+3].bitmap,14,84,ret);
	blitAt(luck22->ourImages[curh->getCurrentLuck()+3].bitmap,14,101,ret);
	return ret;
}
SDL_Surface * Graphics::drawTownInfoWin(const CGTownInstance * curh)
{
	char * buf = new char[10];
	blueToPlayersAdv(tInfo,curh->tempOwner);
	SDL_Surface * ret = SDL_DisplayFormat(tInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	printAt(curh->name,75,15,GEOR13,zwykly,ret);

	int pom = curh->fortLevel() - 1; if(pom<0) pom = 3;
	blitAt(forts->ourImages[pom].bitmap,115,42,ret);
	if((pom=curh->hallLevel())>=0)
		blitAt(halls->ourImages[pom].bitmap,77,42,ret);
	SDL_itoa(curh->dailyIncome(),buf,10);
	printAtMiddle(buf,167,70,GEORM,zwykly,ret);
	for (std::map<si32,std::pair<ui32,si32> >::const_iterator i=curh->army.slots.begin(); i!=curh->army.slots.end();i++)
	{
		if(!i->second.second)
			continue;
		blitAt(graphics->smallImgs[(*i).second.first],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		SDL_itoa((*i).second.second,buf,10);
		printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+39,GEORM,zwykly,ret);
	}

	//blit town icon
	pom = curh->subID*2;
	if (!curh->hasFort())
		pom += F_NUMBER*2;
	if(curh->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	if(curh->garrisonHero)
		blitAt(graphics->heroInGarrison,158,87,ret);
	blitAt(bigTownPic->ourImages[pom].bitmap,13,13,ret);
	delete[] buf;
	return ret;
}

void Graphics::loadPaletteAndColors()
{
	std::string pals = CGI->bitmaph->getTextFile("PLAYERS.PAL");
	playerColorPalette = new SDL_Color[256];
	neutralColor = new SDL_Color;
	playerColors = new SDL_Color[PLAYER_LIMIT];
	int startPoint = 24; //beginning byte; used to read
	for(int i=0; i<256; ++i)
	{
		SDL_Color col;
		col.r = pals[startPoint++];
		col.g = pals[startPoint++];
		col.b = pals[startPoint++];
		col.unused = pals[startPoint++];
		playerColorPalette[i] = col;
	}
	//colors initialization
	int3 kolory[] = {int3(0xff,0,0),int3(0x31,0x52,0xff),int3(0x9c,0x73,0x52),int3(0x42,0x94,0x29),
		int3(0xff,0x84,0x0),int3(0x8c,0x29,0xa5),int3(0x09,0x9c,0xa5),int3(0xc6,0x7b,0x8c)};
	for(int i=0;i<8;i++)
	{
		playerColors[i].r = kolory[i].x;
		playerColors[i].g = kolory[i].y;
		playerColors[i].b = kolory[i].z;
		playerColors[i].unused = 0;
	}
	neutralColor->r = 0x84; neutralColor->g = 0x84; neutralColor->b = 0x84; neutralColor->unused = 0x0;//gray

	std::ifstream bback("config/mageBg.txt");
	while(!bback.eof())
	{
		bback >> pals;
		guildBgs.push_back(pals);
	}
	bback.close();
}	
void Graphics::initializeBattleGraphics()
{
	std::ifstream bback("config/battleBack.txt");
	battleBacks.resize(9);
	for(int i=0; i<9; ++i) //9 - number of terrains battle can be fought on
	{
		int am;
		bback>>am;
		battleBacks[i].resize(am);
		for(int f=0; f<am; ++f)
		{
			bback>>battleBacks[i][f];
		}
	}

	//initializing battle hero animation
	std::ifstream bher("config/battleHeroes.txt");
	int numberofh;
	bher>>numberofh;
	battleHeroes.resize(numberofh);
	for(int i=0; i<numberofh; ++i) //9 - number of terrains battle can be fought on
	{
		bher>>battleHeroes[i];
	}
}
Graphics::Graphics()
{
	slotsPos.push_back(std::pair<int,int>(44,82));
	slotsPos.push_back(std::pair<int,int>(80,82));
	slotsPos.push_back(std::pair<int,int>(116,82));
	slotsPos.push_back(std::pair<int,int>(26,131));
	slotsPos.push_back(std::pair<int,int>(62,131));
	slotsPos.push_back(std::pair<int,int>(98,131));
	slotsPos.push_back(std::pair<int,int>(134,131));

	CDefHandler *smi, *smi2;

	std::vector<Task> tasks; //preparing list of graphics to load
	tasks += boost::bind(&Graphics::loadPaletteAndColors,this);
	tasks += boost::bind(&Graphics::loadHeroFlags,this);
	tasks += boost::bind(&Graphics::loadHeroPortraits,this);
	tasks += boost::bind(&Graphics::initializeBattleGraphics,this);
	tasks += GET_SURFACE(hInfo,"HEROQVBK.bmp");
	tasks += GET_SURFACE(tInfo,"TOWNQVBK.bmp");
	tasks += GET_SURFACE(heroInGarrison,"TOWNQKGH.bmp");
	tasks += GET_DEF(artDefs,"ARTIFACT.DEF");
	tasks += GET_DEF_ESS(forts,"ITMCLS.DEF");
	tasks += GET_DEF_ESS(luck22,"ILCK22.DEF");
	tasks += GET_DEF_ESS(luck30,"ILCK30.DEF");
	tasks += GET_DEF_ESS(luck42,"ILCK42.DEF");
	tasks += GET_DEF_ESS(luck82,"ILCK82.DEF");
	tasks += GET_DEF_ESS(morale22,"IMRL22.DEF");
	tasks += GET_DEF_ESS(morale30,"IMRL30.DEF");
	tasks += GET_DEF_ESS(morale42,"IMRL42.DEF");
	tasks += GET_DEF_ESS(morale82,"IMRL82.DEF");
	tasks += GET_DEF_ESS(halls,"ITMTLS.DEF");
	tasks += GET_DEF_ESS(bigTownPic,"ITPT.DEF");
	tasks += GET_DEF(pskillsb,"PSKILL.DEF");
	tasks += GET_DEF(pskillsm,"PSKIL42.DEF");
	tasks += GET_DEF(resources,"RESOUR82.DEF");
	tasks += GET_DEF(un44,"UN44.DEF");
	tasks += GET_DEF(smallIcons,"ITPA.DEF");
	tasks += GET_DEF(resources32,"RESOURCE.DEF");
	tasks += GET_DEF(smi,"CPRSMALL.DEF");
	tasks += GET_DEF(smi2,"TWCRPORT.DEF");
	tasks += GET_DEF(flags,"CREST58.DEF");

	std::ifstream ifs("config/cr_bgs.txt"); 
	int id;
	std::string name;
	while(!ifs.eof())
	{
		ifs >> id >> name;
		tasks += GET_SURFACE(backgrounds[id],name);
		name.replace(0,5,"TPCAS");
		tasks += GET_SURFACE(backgroundsm[id],name);
	}

	CThreadHelper th(&tasks,std::max((unsigned int)1,boost::thread::hardware_concurrency()));
	th.run();

	//handling 32x32px imgs
	smi->notFreeImgs = true;
	for (int i=0; i<smi->ourImages.size(); i++)
	{
		smallImgs[i-2] = smi->ourImages[i].bitmap;
	}
	delete smi;
	smi2->notFreeImgs = true;
	for (int i=0; i<smi2->ourImages.size(); i++)
	{
		bigImgs[i-2] = smi2->ourImages[i].bitmap;
	}
	delete smi2;
}
void Graphics::loadHeroPortraits()
{	
	std::ifstream of("config/portrety.txt");
	int numberOfPortraits;
	of>>numberOfPortraits;
	for (int j=0; j<numberOfPortraits; j++)
	{
		int ID;
		of>>ID;
		std::string path;
		of>>path;
		portraitSmall.push_back(BitmapHandler::loadBitmap(path));
		for(int ff=0; ff<path.size(); ++ff) //size letter is usually third one, but there are exceptions an it should fix the problem
		{
			if(path[ff]=='S')
			{
				path[ff]='L';
				break;
			}
		}
		portraitLarge.push_back(BitmapHandler::loadBitmap(path));
		SDL_SetColorKey(portraitLarge[portraitLarge.size()-1],SDL_SRCCOLORKEY,SDL_MapRGB(portraitLarge[portraitLarge.size()-1]->format,0,255,255));

	}
	of.close();
}
void Graphics::loadHeroAnim(std::vector<CDefHandler **> & anims)
{
	std::vector<std::pair<int,int> > rotations; //first - group number to be rotated1, second - group number after rotation1
	rotations += std::make_pair(6,10), std::make_pair(7,11), std::make_pair(8,12), std::make_pair(1,13),
		std::make_pair(2,14), std::make_pair(3,15);
	for(int i=0; i<anims.size();i++)
	{
		std::stringstream nm;
		nm << "AH" << std::setw(2) << std::setfill('0') << i << "_.DEF";
		std::string name = nm.str();
		(*anims[i]) = CDefHandler::giveDef(name);
		int pom = 0; //how many groups has been rotated
		for(int o=7; pom<6; ++o)
		{
			for(int p=0;p<6;p++)
			{
				if((*anims[i])->ourImages[o].groupNumber==rotations[p].first)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01((*anims[i])->ourImages[o+e].bitmap);
						nci.groupNumber = rotations[p].second;
						nci.imName = std::string();
						(*anims[i])->ourImages.push_back(nci);
						if(pom>2) //we need only one frame for groups 13/14/15
							break;
					}
					if(pom<3) //there are eight frames of animtion of groups 6/7/8 so for speed we'll skip them
						o+=8;
					else //there is only one frame of 1/2/3
						o+=1;
					++pom;
					if(p==2 && pom<4) //group1 starts at index 1
						o = 1;
				}
			}
		}
		for(int ff=0; ff<(*anims[i])->ourImages.size(); ++ff)
		{
			CSDL_Ext::alphaTransform((*anims[i])->ourImages[ff].bitmap);
		}
		(*anims[i])->alphaTransformed = true;
	}
}

void Graphics::loadHeroFlags(std::pair<std::vector<CDefHandler *> Graphics::*, std::vector<const char *> > &pr, bool mode)
{
	for(int i=0;i<8;i++)
		(this->*pr.first).push_back(CDefHandler::giveDef(pr.second[i]));
	std::vector<std::pair<int,int> > rotations; //first - group number to be rotated1, second - group number after rotation1
	rotations += std::make_pair(6,10), std::make_pair(7,11), std::make_pair(8,12);
	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<(this->*pr.first)[q]->ourImages.size(); ++o)
		{
			for(int p=0;p<rotations.size();p++)
			{
				if((this->*pr.first)[q]->ourImages[o].groupNumber==rotations[p].first)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01((this->*pr.first)[q]->ourImages[o+e].bitmap);
						nci.groupNumber = rotations[p].second;
						nci.imName = std::string();
						(this->*pr.first)[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}
		}
		if (mode)
		{
			for(int o=0; o<flags4[q]->ourImages.size(); ++o)
			{
				if(flags4[q]->ourImages[o].groupNumber==1)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
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
						nci.groupNumber = 15;
						nci.imName = std::string();
						flags4[q]->ourImages.push_back(nci);
					}
					o+=8;
				}
			}
		}
		for(int ff=0; ff<(this->*pr.first)[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey((this->*pr.first)[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB((this->*pr.first)[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		(this->*pr.first)[q]->alphaTransformed = true;
	}
}

void Graphics::loadHeroFlags()
{
	using namespace boost::assign;
	timeHandler th;
	std::vector<CDefHandler *> Graphics::*point;
	std::pair<std::vector<CDefHandler *> Graphics::*, std::vector<const char *> > pr[4];
	pr[0].first = &Graphics::flags1;
	pr[0].second+=("ABF01L.DEF"),("ABF01G.DEF"),("ABF01R.DEF"),("ABF01D.DEF"),("ABF01B.DEF"),
		("ABF01P.DEF"),("ABF01W.DEF"),("ABF01K.DEF");
	pr[1].first = &Graphics::flags2;
	pr[1].second+=("ABF02L.DEF"),("ABF02G.DEF"),("ABF02R.DEF"),("ABF02D.DEF"),("ABF02B.DEF"),
		("ABF02P.DEF"),("ABF02W.DEF"),("ABF02K.DEF");
	pr[2].first = &Graphics::flags3;
	pr[2].second+=("ABF03L.DEF"),("ABF03G.DEF"),("ABF03R.DEF"),("ABF03D.DEF"),("ABF03B.DEF"),
		("ABF03P.DEF"),("ABF03W.DEF"),("ABF03K.DEF");
	pr[3].first = &Graphics::flags4;
	pr[3].second+=("AF00.DEF"),("AF01.DEF"),("AF02.DEF"),("AF03.DEF"),("AF04.DEF"),
		("AF05.DEF"),("AF06.DEF"),("AF07.DEF");
	boost::thread_group grupa;
	grupa.create_thread(boost::bind(&Graphics::loadHeroFlags,this,boost::ref(pr[3]),true));
	grupa.create_thread(boost::bind(&Graphics::loadHeroFlags,this,boost::ref(pr[2]),false));
	grupa.create_thread(boost::bind(&Graphics::loadHeroFlags,this,boost::ref(pr[1]),false));
	grupa.create_thread(boost::bind(&Graphics::loadHeroFlags,this,boost::ref(pr[0]),false));
	grupa.join_all();
	tlog0 << "Loading and transforming heroes' flags: "<<th.getDif()<<std::endl;
}
SDL_Surface * Graphics::getPic(int ID, bool fort, bool builded)
{
	if (ID==-1)
		return smallIcons->ourImages[0].bitmap;
	else if (ID==-2)
		return smallIcons->ourImages[1].bitmap;
	else if (ID==-3)
		return smallIcons->ourImages[2+F_NUMBER*4].bitmap;
	else if (ID>F_NUMBER || ID<-3)
#ifndef __GNUC__
		throw new std::exception("Invalid ID");
#else
		throw new std::exception();
#endif
	else
	{
		int pom = 3;
		if(!fort)
			pom+=F_NUMBER*2;
		pom += ID*2;
		if (!builded)
			pom--;
		return smallIcons->ourImages[pom].bitmap;
	}
}

void Graphics::blueToPlayersAdv(SDL_Surface * sur, int player)
{
	if(player==1) //it is actually blue...
		return;
	if(sur->format->BitsPerPixel == 8)
	{
		for(int i=0; i<32; ++i)
		{
			sur->format->palette->colors[224+i] = playerColorPalette[32*player+i];
		}
	}
	else if(sur->format->BitsPerPixel == 24) //should never happen in general
	{
		for(int y=0; y<sur->h; ++y)
		{
			for(int x=0; x<sur->w; ++x)
			{
				Uint8* cp = (Uint8*)sur->pixels + y*sur->pitch + x*3;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					if(cp[2]>cp[1] && cp[2]>cp[0])
					{
						std::vector<long long int> sort1;
						sort1.push_back(cp[0]);
						sort1.push_back(cp[1]);
						sort1.push_back(cp[2]);
						std::vector< std::pair<long long int, Uint8*> > sort2;
						sort2.push_back(std::make_pair(graphics->playerColors[player].r, &(cp[0])));
						sort2.push_back(std::make_pair(graphics->playerColors[player].g, &(cp[1])));
						sort2.push_back(std::make_pair(graphics->playerColors[player].b, &(cp[2])));
						std::sort(sort1.begin(), sort1.end());
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						if(sort2[1].first>sort2[2].first)
							std::swap(sort2[1], sort2[2]);
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						for(int hh=0; hh<3; ++hh)
						{
							(*sort2[hh].second) = (sort1[hh] + sort2[hh].first)/2.2;
						}
					}
				}
				else
				{
					if(
						(/*(mode==0) && (cp[0]>cp[1]) && (cp[0]>cp[2])) ||
						((mode==1) &&*/ (cp[2]<45) && (cp[0]>80) && (cp[1]<70) && ((cp[0]-cp[1])>40))
					  )
					{
						std::vector<long long int> sort1;
						sort1.push_back(cp[2]);
						sort1.push_back(cp[1]);
						sort1.push_back(cp[0]);
						std::vector< std::pair<long long int, Uint8*> > sort2;
						sort2.push_back(std::make_pair(graphics->playerColors[player].r, &(cp[2])));
						sort2.push_back(std::make_pair(graphics->playerColors[player].g, &(cp[1])));
						sort2.push_back(std::make_pair(graphics->playerColors[player].b, &(cp[0])));
						std::sort(sort1.begin(), sort1.end());
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						if(sort2[1].first>sort2[2].first)
							std::swap(sort2[1], sort2[2]);
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						for(int hh=0; hh<3; ++hh)
						{
							(*sort2[hh].second) = (sort1[hh]*0.8 + sort2[hh].first)/2;
						}
					}
				}
			}
		}
	}
}
