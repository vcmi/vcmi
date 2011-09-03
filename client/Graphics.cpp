#include "../stdafx.h"
#include "Graphics.h"
#include "CDefHandler.h"
#include "SDL_Extensions.h"
#include <SDL_ttf.h>
#include <boost/assign/std/vector.hpp> 
#include <sstream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include "../CThreadHelper.h"
#include "CGameInfo.h"
#include "../lib/CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../CCallback.h"
#include "../lib/CTownHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CCreatureHandler.h"
#include "CBitmapHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CGameState.h"
#include "../lib/JsonNode.h"

using namespace boost::assign;
using namespace CSDL_Ext;
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/*
 * Graphics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

Graphics * graphics = NULL;

SDL_Surface * Graphics::drawHeroInfoWin(const InfoAboutHero &curh)
{
	char buf[15];
	blueToPlayersAdv(hInfo,curh.owner);
	SDL_Surface * ret = SDL_DisplayFormat(hInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));

	printAt(curh.name,75,13,FONT_SMALL,zwykly,ret); //name
	blitAt(graphics->portraitLarge[curh.portrait],11,12,ret); //portrait

	//army
	for (ArmyDescriptor::const_iterator i = curh.army.begin(); i!=curh.army.end();i++)
	{
		blitAt(graphics->smallImgs[i->second.type->idNumber],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		if(curh.details)
		{
			SDL_itoa((*i).second.count,buf,10);
			printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+41,FONT_TINY,zwykly,ret);
		}
		else
		{
			printAtMiddle(VLC->generaltexth->arraytxt[174 + 3*(i->second.count)],slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+41,FONT_TINY,zwykly,ret);
		}
	}

	if(curh.details)
	{
		for (int i = 0; i < PRIMARY_SKILLS; i++)
		{
			SDL_itoa(curh.details->primskills[i], buf, 10);
			printAtMiddle(buf,84+28*i,70,FONT_SMALL,zwykly,ret);
		}

		//mana points
		SDL_itoa(curh.details->mana,buf,10);
		printAtMiddle(buf,167,108,FONT_TINY,zwykly,ret); 

		blitAt(morale22->ourImages[curh.details->morale+3].bitmap,14,84,ret);	//luck
		blitAt(luck22->ourImages[curh.details->morale+3].bitmap,14,101,ret);	//morale
	}
	return ret;
}

SDL_Surface * Graphics::drawHeroInfoWin(const CGHeroInstance * curh)
{
	InfoAboutHero iah;
	iah.initFromHero(curh, true);
	return drawHeroInfoWin(iah);
}

SDL_Surface * Graphics::drawTownInfoWin(const CGTownInstance * curh)
{
	InfoAboutTown iah;
	iah.initFromTown(curh, true);
	return drawTownInfoWin(iah);
}

SDL_Surface * Graphics::drawTownInfoWin( const InfoAboutTown & curh )
{
	char buf[10];
	blueToPlayersAdv(tInfo,curh.owner);
	SDL_Surface * ret = SDL_DisplayFormat(tInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));

	printAt(curh.name,75,12,FONT_SMALL,zwykly,ret); //name
	int pom = curh.fortLevel - 1; if(pom<0) pom = 3; //fort pic id
	blitAt(forts->ourImages[pom].bitmap,115,42,ret); //fort

	for (ArmyDescriptor::const_iterator i=curh.army.begin(); i!=curh.army.end();i++)
	{
		//if(!i->second.second)
		//	continue;
		blitAt(graphics->smallImgs[(*i).second.type->idNumber],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		if(curh.details)
		{
			// Show exact creature amount.
			SDL_itoa((*i).second.count,buf,10);
			printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+41,FONT_TINY,zwykly,ret);
		}
		else
		{
			// Show only a rough amount for creature stacks.
			// TODO: Deal with case when no information at all about size shold be presented.
			std::string roughAmount = curh.obj->getRoughAmount(i->first);
			printAtMiddle(roughAmount,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+41,FONT_TINY,zwykly,ret);
		}
	}

	//blit town icon
	if (curh.tType) {
		pom = curh.tType->typeID*2;
		if (!curh.fortLevel)
			pom += F_NUMBER*2;
		if(curh.built)
			pom++;
		blitAt(bigTownPic->ourImages[pom].bitmap,13,13,ret);
	}

	if(curh.details)
	{
		//hall level icon
		if((pom=curh.details->hallLevel) >= 0)
			blitAt(halls->ourImages[pom].bitmap, 77, 42, ret);

		if (curh.details->goldIncome >= 0) {
			SDL_itoa(curh.details->goldIncome, buf, 10); //gold income
			printAtMiddle(buf, 167, 70, FONT_TINY, zwykly, ret);
		}
		if(curh.details->garrisonedHero) //garrisoned hero icon
			blitAt(graphics->heroInGarrison,158,87,ret);
	}

	return ret;
}

void Graphics::loadPaletteAndColors()
{
	std::string pals = bitmaph->getTextFile("PLAYERS.PAL", FILE_OTHER);
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

	neutralColorPalette = new SDL_Color[32];
	std::ifstream ncp;
	ncp.open(DATA_DIR "/config/NEUTRAL.PAL", std::ios::binary);
	for(int i=0; i<32; ++i)
	{
		ncp.read((char*)&neutralColorPalette[i].r,1);
		ncp.read((char*)&neutralColorPalette[i].g,1);
		ncp.read((char*)&neutralColorPalette[i].b,1);
		ncp.read((char*)&neutralColorPalette[i].unused,1);
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
	const JsonNode config(DATA_DIR "/config/town_pictures.json");
	BOOST_FOREACH(const JsonNode &p, config["town_pictures"].Vector()) {

		townBgs.push_back(p["town_background"].String());
		guildBgs.push_back(p["guild_background"].String());
		buildingPics.push_back(p["building_picture"].String());
	}
}

void Graphics::initializeBattleGraphics()
{
	const JsonNode config(DATA_DIR "/config/battles_graphics.json");
	
	// Reserve enough space for the terrains
	// TODO: there should be a method to count the number of elements
	int idx = 0;
	BOOST_FOREACH(const JsonNode &t, config["backgrounds"].Vector()) {
		idx++;
	}
	battleBacks.resize(idx+1);	// 1 to idx, 0 is unused

	idx = 1;
	BOOST_FOREACH(const JsonNode &t, config["backgrounds"].Vector()) {
		battleBacks[idx].push_back(t.String());
		idx++;
	}

	//initializing battle hero animation
	idx = 0;
	BOOST_FOREACH(const JsonNode &h, config["heroes"].Vector()) {
		idx++;
	}
	battleHeroes.resize(idx);

	idx = 0;
	BOOST_FOREACH(const JsonNode &h, config["heroes"].Vector()) {
		battleHeroes[idx] = h.String();
		idx ++;
	}

	//initialization of AC->def name mapping
	BOOST_FOREACH(const JsonNode &ac, config["ac_mapping"].Vector()) {
		int ACid = ac["id"].Float();
		std::vector< std::string > toAdd;

		BOOST_FOREACH(const JsonNode &defname, ac["defnames"].Vector()) {
			toAdd.push_back(defname.String());
		}

		battleACToDef[ACid] = toAdd;
	}

	spellEffectsPics = CDefHandler::giveDefEss("SpellInt.def");
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
	tasks += boost::bind(&Graphics::loadFonts,this);
	tasks += boost::bind(&Graphics::loadTrueType,this);
	tasks += boost::bind(&Graphics::loadPaletteAndColors,this);
	tasks += boost::bind(&Graphics::loadHeroFlags,this);
	tasks += boost::bind(&Graphics::loadHeroPortraits,this);
	tasks += boost::bind(&Graphics::initializeBattleGraphics,this);
	tasks += boost::bind(&Graphics::loadWallPositions,this);
	tasks += boost::bind(&Graphics::loadErmuToPicture,this);
	tasks += GET_SURFACE(hInfo,"HEROQVBK.bmp");
	tasks += GET_SURFACE(tInfo,"TOWNQVBK.bmp");
	tasks += GET_SURFACE(heroInGarrison,"TOWNQKGH.bmp");
	tasks += GET_DEF_ESS(artDefs,"ARTIFACT.DEF");
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
	tasks += GET_DEF_ESS(pskillsb,"PSKILL.DEF");
	tasks += GET_DEF_ESS(pskillsm,"PSKIL42.DEF");
	tasks += GET_DEF_ESS(pskillst,"PSKIL32.DEF");
	tasks += GET_DEF_ESS(resources,"RESOUR82.DEF");
	tasks += GET_DEF_ESS(un32,"UN32.DEF");
	tasks += GET_DEF_ESS(un44,"UN44.DEF");
	tasks += GET_DEF_ESS(smallIcons,"ITPA.DEF");
	tasks += GET_DEF_ESS(resources32,"RESOURCE.DEF");
	tasks += GET_DEF(smi,"CPRSMALL.DEF");
	tasks += GET_DEF(smi2,"TWCRPORT.DEF");
	tasks += GET_DEF_ESS(flags,"CREST58.DEF");
	tasks += GET_DEF_ESS(abils32,"SECSK32.DEF");
	tasks += GET_DEF_ESS(abils44,"SECSKILL.DEF");
	tasks += GET_DEF_ESS(abils82,"SECSK82.DEF");
	tasks += GET_DEF_ESS(spellscr,"SPELLSCR.DEF");
	tasks += GET_DEF_ESS(heroMoveArrows,"ADAG.DEF");

	const JsonNode config(DATA_DIR "/config/creature_backgrounds.json");
	BOOST_FOREACH(const JsonNode &b, config["backgrounds"].Vector()) {
		const int id = b["id"].Float();
		tasks += GET_SURFACE(backgrounds[id], b["bg130"].String());
		tasks += GET_SURFACE(backgroundsm[id], b["bg120"].String());
	}

	CThreadHelper th(&tasks,std::max((unsigned int)1,boost::thread::hardware_concurrency()));
	th.run();

	for(size_t y=0; y < heroMoveArrows->ourImages.size(); ++y)
	{
		CSDL_Ext::alphaTransform(heroMoveArrows->ourImages[y].bitmap);
	}

	//handling 32x32px imgs
	smi->notFreeImgs = true;
	for (size_t i=0; i<smi->ourImages.size(); ++i)
	{
		smallImgs[i-2] = smi->ourImages[i].bitmap;
	}
	delete smi;
	smi2->notFreeImgs = true;
	for (size_t i=0; i<smi2->ourImages.size(); ++i)
	{
		bigImgs[i-2] = smi2->ourImages[i].bitmap;
	}
	//hack for green color on big infernal troglodite image - Mantis #758
	SDL_Color green = {0x30, 0x5c, 0x20, SDL_ALPHA_OPAQUE};
	bigImgs[71]->format->palette->colors[7] = green;
	delete smi2;
}
void Graphics::loadHeroPortraits()
{	
	const JsonNode config(DATA_DIR "/config/portraits.json");

	BOOST_FOREACH(const JsonNode &portrait_node, config["hero_portrait"].Vector()) {
		std::string filename = portrait_node["filename"].String();

		/* Small portrait. */
		portraitSmall.push_back(BitmapHandler::loadBitmap(filename));

		/* Large portrait. Alter the filename. Size letter is usually
		 * third one, but there are exceptions and it should fix the
		 * problem. */
		for (int ff=0; ff<filename.size(); ++ff)
		{
			if (filename[ff]=='S') {
				filename[ff]='L';
				break;
			}
		}
		portraitLarge.push_back(BitmapHandler::loadBitmap(filename));

		SDL_SetColorKey(portraitLarge[portraitLarge.size()-1],SDL_SRCCOLORKEY,SDL_MapRGB(portraitLarge[portraitLarge.size()-1]->format,0,255,255));
	}
}

void Graphics::loadWallPositions()
{
	const JsonNode config(DATA_DIR "/config/wall_pos.json");

	BOOST_FOREACH(const JsonNode &town, config["towns"].Vector()) {
		int townID = town["id"].Float();

		BOOST_FOREACH(const JsonNode &coords, town["pos"].Vector()) {
			Point pt(coords["x"].Float(), coords["y"].Float());
			wallPositions[townID].push_back(pt);
		}

		assert(wallPositions[townID].size() == 21);
	}
}

void Graphics::loadHeroAnims()
{
	std::vector<std::pair<int,int> > rotations; //first - group number to be rotated1, second - group number after rotation1
	rotations += std::make_pair(6,10), std::make_pair(7,11), std::make_pair(8,12), std::make_pair(1,13),
		std::make_pair(2,14), std::make_pair(3,15);
	for(size_t i=0; i<F_NUMBER * 2; ++i)
	{
		std::ostringstream nm;
		nm << "AH" << std::setw(2) << std::setfill('0') << i << "_.DEF";
		loadHeroAnim(nm.str(), rotations, &Graphics::heroAnims);
		std::string name = nm.str();
	}	

	loadHeroAnim("AB01_.DEF", rotations, &Graphics::boatAnims);
	loadHeroAnim("AB02_.DEF", rotations, &Graphics::boatAnims);
	loadHeroAnim("AB03_.DEF", rotations, &Graphics::boatAnims);
}

void Graphics::loadHeroAnim( const std::string &name, const std::vector<std::pair<int,int> > &rotations, std::vector<CDefEssential *> Graphics::*dst )
{
	CDefEssential *anim = CDefHandler::giveDefEss(name);
	(this->*dst).push_back(anim);
	int pom = 0; //how many groups has been rotated
	for(int o=7; pom<6; ++o)
	{
		for(int p=0;p<6;p++)
		{
			if(anim->ourImages[o].groupNumber == rotations[p].first)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(anim->ourImages[o+e].bitmap);
					nci.groupNumber = rotations[p].second;
					nci.imName = std::string();
					anim->ourImages.push_back(nci);
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
	for(size_t ff=0; ff<anim->ourImages.size(); ++ff)
	{
		CSDL_Ext::alphaTransform(anim->ourImages[ff].bitmap);
	}
}

void Graphics::loadHeroFlags(std::pair<std::vector<CDefEssential *> Graphics::*, std::vector<const char *> > &pr, bool mode)
{
	for(int i=0;i<8;i++)
		(this->*pr.first).push_back(CDefHandler::giveDefEss(pr.second[i]));
	std::vector<std::pair<int,int> > rotations; //first - group number to be rotated1, second - group number after rotation1
	rotations += std::make_pair(6,10), std::make_pair(7,11), std::make_pair(8,12);
	for(int q=0; q<8; ++q)
	{
		std::vector<Cimage> &curImgs = (this->*pr.first)[q]->ourImages;
		for(size_t o=0; o<curImgs.size(); ++o)
		{
			for(size_t p=0; p<rotations.size(); p++)
			{
				if(curImgs[o].groupNumber==rotations[p].first)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(curImgs[o+e].bitmap);
						nci.groupNumber = rotations[p].second;
						nci.imName = std::string();
						curImgs.push_back(nci);
					}
					o+=8;
				}
			}
		}
		if (mode)
		{
			for(size_t o=0; o<curImgs.size(); ++o)
			{
				if(curImgs[o].groupNumber==1 || curImgs[o].groupNumber==2 || curImgs[o].groupNumber==3)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::rotate01(curImgs[o+e].bitmap);
						nci.groupNumber = 12 + curImgs[o].groupNumber;
						nci.imName = std::string();
						curImgs.push_back(nci);
					}
					o+=8;
				}
			}
		}
		for(size_t ff=0; ff<curImgs.size(); ++ff)
		{
			SDL_SetColorKey(curImgs[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(curImgs[ff].bitmap->format, 0, 255, 255)
				);
		}
	}
}

void Graphics::loadHeroFlags()
{
	using namespace boost::assign;
	timeHandler th;
	std::pair<std::vector<CDefEssential *> Graphics::*, std::vector<const char *> > pr[4];
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
	for(int g=3; g>=0; --g)
	{
		grupa.create_thread(boost::bind(&Graphics::loadHeroFlags,this,boost::ref(pr[g]),true));
	}
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
// 	if(player==1) //it is actually blue...
// 		return;
	if(sur->format->BitsPerPixel == 8)
	{
		SDL_Color *palette = NULL;
		if(player < PLAYER_LIMIT && player >= 0)
		{
			palette = playerColorPalette + 32*player;
		}
		else if(player == 255 || player == -1)
		{
			palette = neutralColorPalette;
		}
		else
		{
			tlog1 << "Wrong player id in blueToPlayersAdv (" << player << ")!\n";
			return;
		}

		SDL_SetColors(sur, palette, 224, 32);
		//for(int i=0; i<32; ++i)
		//{
		//	sur->format->palette->colors[224+i] = palette[i];
		//}
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

void Graphics::loadTrueType()
{
	bool ttfPresent = false;//was TTF initialised or not
	for(int i = 0; i < FONTS_NUMBER; i++)
		fontsTrueType[i] = NULL;
	std::ifstream ff(DATA_DIR "/config/fonts.txt");
	while(!ff.eof())
	{
		int enabl, fntID, fntSize;
		std::string fntName;

		ff >> enabl;//enabled font or not
		if (enabl==-1)
			break;//end of data
		ff >> fntID;//what font will be replaced
		ff >> fntName;//name of truetype font
		ff >> fntSize;//size of font
		if (enabl)
		{
			if (!ttfPresent)
			{
				ttfPresent = true;
				TTF_Init();
				atexit(TTF_Quit);
			};
			fntName = DATA_DIR + ( "/Fonts/" + fntName);
			fontsTrueType[fntID] = TTF_OpenFont(fntName.c_str(),fntSize);
		}
	}
	ff.close();
	ff.clear();
}

Font * Graphics::loadFont( const char * name )
{
	int len = 0;
	unsigned char * hlp = bitmaph->giveFile(name, FILE_FONT, &len);
	if(!hlp || !len)
	{
		tlog1 << "Error: cannot load font: " << name << std::endl;
		return NULL;
	}

	int magic =  *(const int*)hlp;
	if(len < 10000  || (magic != 589598 && magic != 589599))
	{
		tlog1 << "Suspicious font file (length " << len <<", fname " << name << "), logging to suspicious_" << name << ".fnt\n";
		std::string suspFName = "suspicious_" + std::string(name) + ".fnt";
		std::ofstream o(suspFName.c_str());
		o.write((const char*)hlp, len);
	}

	Font *ret = new Font(hlp);
	return ret;
}

void Graphics::loadFonts()
{
	static const char *fontnames [] = {"BIGFONT.FNT", "CALLI10R.FNT", "CREDITS.FNT", "HISCORE.FNT", "MEDFONT.FNT",
								"SMALFONT.FNT", "TIMES08R.FNT", "TINY.FNT", "VERD10B.FNT"} ;

	assert(ARRAY_COUNT(fontnames) == FONTS_NUMBER);
	for(int i = 0; i < FONTS_NUMBER; i++)
		fonts[i] = loadFont(fontnames[i]);
}

CDefEssential * Graphics::getDef( const CGObjectInstance * obj )
{
	return advmapobjGraphics[obj->defInfo->id][obj->defInfo->subid];
}

CDefEssential * Graphics::getDef( const CGDefInfo * info )
{
	return advmapobjGraphics[info->id][info->subid];
}

void Graphics::loadErmuToPicture()
{
	//loading ERMU to picture
	const JsonNode config(DATA_DIR "/config/ERMU_to_picture.json");
	int etp_idx = 0;
	BOOST_FOREACH(const JsonNode &etp, config["ERMU_to_picture"].Vector()) {
		int idx = 0;
		BOOST_FOREACH(const JsonNode &n, etp.Vector()) {
			ERMUtoPicture[idx][etp_idx] = n.String();
			idx ++;
		}
		assert (idx == ARRAY_COUNT(ERMUtoPicture));

		etp_idx ++;
	}
	assert (etp_idx == 44);
}

Font::Font(unsigned char *Data)
{
	data = Data;
	int i = 0;

	height = data[5];

	i = 32;
	for(int ci = 0; ci < 256; ci++)
	{
		chars[ci].unknown1 = readNormalNr(data, i); i+=4;
		chars[ci].width = readNormalNr(data, i); i+=4;
		chars[ci].unknown2 =readNormalNr(data, i); i+=4;

		//if(ci>=30)
		//	tlog0 << ci << ". (" << (char)ci << "). Width: " << chars[ci].width << " U1/U2:" << chars[ci].unknown1 << "/" << chars[ci].unknown2 << std::endl;
	}
	for(int ci = 0; ci < 256; ci++)
	{
		chars[ci].offset = readNormalNr(data, i); i+=4;
		chars[ci].pixels = data + 4128 + chars[ci].offset;
	}
}

Font::~Font()
{
	delete [] data;
}

int Font::getWidth(const char *text ) const
{
	int length = std::strlen(text);
	int ret = 0;

	for(int i = 0; i < length; i++)
	{
		unsigned char c = text[i];
		ret += chars[c].width + chars[c].unknown1 + chars[c].unknown2;
	}

	return ret;
}

int Font::getCharWidth( char c ) const
{
	const Char &C = chars[(unsigned char)c];
	return C.width + C.unknown1 + C.unknown2;;
}

/*
void Font::WriteAt(const char *text, SDL_Surface *sur, int x, int y )
{
	 SDL_Surface *SDL_CreateRGBSurfaceFrom(pixels, w, h, 8, int pitch,
                        224, 28, 3, 0);
}
*/
