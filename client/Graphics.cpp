#include "StdInc.h"
#include "Graphics.h"

#include "../lib/filesystem/CResourceLoader.h"
#include "CDefHandler.h"
#include "gui/SDL_Extensions.h"
#include <SDL_ttf.h>
#include "../lib/CThreadHelper.h"
#include "CGameInfo.h"
#include "../lib/VCMI_Lib.h"
#include "../CCallback.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CCreatureHandler.h"
#include "CBitmapHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CGameState.h"
#include "../lib/JsonNode.h"
#include "../lib/vcmi_endian.h"
#include "../lib/GameConstants.h"
#include "../lib/CStopWatch.h"

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

void Graphics::loadPaletteAndColors()
{
	auto textFile = CResourceHandler::get()->loadData(ResourceID("DATA/PLAYERS.PAL"));
	std::string pals((char*)textFile.first.get(), textFile.second);

	playerColorPalette = new SDL_Color[256];
	neutralColor = new SDL_Color;
	playerColors = new SDL_Color[PlayerColor::PLAYER_LIMIT_I];
	int startPoint = 24; //beginning byte; used to read
	for(int i=0; i<256; ++i)
	{
		SDL_Color col;
		col.r = pals[startPoint++];
		col.g = pals[startPoint++];
		col.b = pals[startPoint++];
		col.unused = 255;
		startPoint++;
		playerColorPalette[i] = col;
	}

	neutralColorPalette = new SDL_Color[32];
	std::ifstream ncp;
	ncp.open(CResourceHandler::get()->getResourceName(ResourceID("config/NEUTRAL.PAL")), std::ios::binary);
	for(int i=0; i<32; ++i)
	{
		ncp.read((char*)&neutralColorPalette[i].r,1);
		ncp.read((char*)&neutralColorPalette[i].g,1);
		ncp.read((char*)&neutralColorPalette[i].b,1);
		ncp.read((char*)&neutralColorPalette[i].unused,1);
		neutralColorPalette[i].unused = !neutralColorPalette[i].unused;
	}


	//colors initialization
	int3 kolory[] = {int3(0xff,0,0),int3(0x31,0x52,0xff),int3(0x9c,0x73,0x52),int3(0x42,0x94,0x29),
		int3(0xff,0x84,0x0),int3(0x8c,0x29,0xa5),int3(0x09,0x9c,0xa5),int3(0xc6,0x7b,0x8c)};
	for(int i=0;i<8;i++)
	{
		playerColors[i].r = kolory[i].x;
		playerColors[i].g = kolory[i].y;
		playerColors[i].b = kolory[i].z;
		playerColors[i].unused = 255;
	}
	neutralColor->r = 0x84; neutralColor->g = 0x84; neutralColor->b = 0x84; neutralColor->unused = 255;//gray
}

void Graphics::initializeBattleGraphics()
{
	const JsonNode config(ResourceID("config/battles_graphics.json"));
	
	// Reserve enough space for the terrains
	int idx = config["backgrounds"].Vector().size();
	battleBacks.resize(idx+1);	// 1 to idx, 0 is unused

	idx = 1;
	BOOST_FOREACH(const JsonNode &t, config["backgrounds"].Vector()) {
		battleBacks[idx].push_back(t.String());
		idx++;
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
	std::vector<Task> tasks; //preparing list of graphics to load
	tasks += boost::bind(&Graphics::loadFonts,this);
	tasks += boost::bind(&Graphics::loadPaletteAndColors,this);
	tasks += boost::bind(&Graphics::loadHeroFlags,this);
	tasks += boost::bind(&Graphics::initializeBattleGraphics,this);
	tasks += boost::bind(&Graphics::loadErmuToPicture,this);
	tasks += GET_DEF_ESS(resources32,"RESOURCE.DEF");
	tasks += GET_DEF_ESS(spellscr,"SPELLSCR.DEF");
	tasks += GET_DEF_ESS(heroMoveArrows,"ADAG.DEF");

	CThreadHelper th(&tasks,std::max((ui32)1,boost::thread::hardware_concurrency()));
	th.run();

	for(size_t y=0; y < heroMoveArrows->ourImages.size(); ++y)
	{
		CSDL_Ext::alphaTransform(heroMoveArrows->ourImages[y].bitmap);
	}
}

void Graphics::loadHeroAnims()
{
	std::vector<std::pair<int,int> > rotations; //first - group number to be rotated1, second - group number after rotation1
	rotations += std::make_pair(6,10), std::make_pair(7,11), std::make_pair(8,12), std::make_pair(1,13),
		std::make_pair(2,14), std::make_pair(3,15);

	for(size_t i=0; i<CGI->heroh->classes.heroClasses.size(); ++i)
	{
		const CHeroClass * hc = CGI->heroh->classes.heroClasses[i];

		if (!vstd::contains(heroAnims, hc->imageMapFemale))
			heroAnims[hc->imageMapFemale] = loadHeroAnim(hc->imageMapFemale, rotations);

		if (!vstd::contains(heroAnims, hc->imageMapMale))
			heroAnims[hc->imageMapMale] = loadHeroAnim(hc->imageMapMale, rotations);
	}

	boatAnims.push_back(loadHeroAnim("AB01_.DEF", rotations));
	boatAnims.push_back(loadHeroAnim("AB02_.DEF", rotations));
	boatAnims.push_back(loadHeroAnim("AB03_.DEF", rotations));
}

CDefEssential * Graphics::loadHeroAnim( const std::string &name, const std::vector<std::pair<int,int> > &rotations)
{
	CDefEssential *anim = CDefHandler::giveDefEss(name);
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
	return anim;
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
	CStopWatch th;
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
    logGlobal->infoStream() << "Loading and transforming heroes' flags: "<<th.getDiff();
}

void Graphics::blueToPlayersAdv(SDL_Surface * sur, PlayerColor player)
{
// 	if(player==1) //it is actually blue...
// 		return;
	if(sur->format->BitsPerPixel == 8)
	{
		SDL_Color *palette = NULL;
		if(player < PlayerColor::PLAYER_LIMIT)
		{
			palette = playerColorPalette + 32*player.getNum();
		}
		else if(player == PlayerColor::NEUTRAL)
		{
			palette = neutralColorPalette;
		}
		else
		{
            logGlobal->errorStream() << "Wrong player id in blueToPlayersAdv (" << player << ")!";
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
						sort2.push_back(std::make_pair(graphics->playerColors[player.getNum()].r, &(cp[0])));
						sort2.push_back(std::make_pair(graphics->playerColors[player.getNum()].g, &(cp[1])));
						sort2.push_back(std::make_pair(graphics->playerColors[player.getNum()].b, &(cp[2])));
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
						sort2.push_back(std::make_pair(graphics->playerColors[player.getNum()].r, &(cp[2])));
						sort2.push_back(std::make_pair(graphics->playerColors[player.getNum()].g, &(cp[1])));
						sort2.push_back(std::make_pair(graphics->playerColors[player.getNum()].b, &(cp[0])));
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

void Graphics::loadFonts()
{
	const JsonNode config(ResourceID("config/fonts.json"));

	const JsonVector & bmpConf = config["bitmap"].Vector();
	const JsonNode   & ttfConf = config["trueType"];

	assert(bmpConf.size() == FONTS_NUMBER);

	for (size_t i=0; i<FONTS_NUMBER; i++)
	{
		std::string filename = bmpConf[i].String();

		if (ttfConf[filename].isNull()) // no ttf override
			fonts[i] = new CBitmapFont(filename);
		else
			fonts[i] = new CTrueTypeFont(ttfConf[filename]);
	}
}

CDefEssential * Graphics::getDef( const CGObjectInstance * obj )
{
	return advmapobjGraphics[obj->defInfo->name];
}

CDefEssential * Graphics::getDef( const CGDefInfo * info )
{
	return advmapobjGraphics[info->name];
}

void Graphics::loadErmuToPicture()
{
	//loading ERMU to picture
	const JsonNode config(ResourceID("config/ERMU_to_picture.json"));
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
