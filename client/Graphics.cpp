#include "StdInc.h"
#include "Graphics.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CBinaryReader.h"
#include "CDefHandler.h"
#include "gui/SDL_Extensions.h"
#include <SDL_ttf.h>
#include "../lib/CThreadHelper.h"
#include "CGameInfo.h"
#include "../lib/VCMI_Lib.h"
#include "../CCallback.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CCreatureHandler.h"
#include "CBitmapHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CGameState.h"
#include "../lib/JsonNode.h"
#include "../lib/vcmi_endian.h"
#include "../lib/GameConstants.h"
#include "../lib/CStopWatch.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/mapObjects/CObjectHandler.h"

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

Graphics * graphics = nullptr;

void Graphics::loadPaletteAndColors()
{
	auto textFile = CResourceHandler::get()->load(ResourceID("DATA/PLAYERS.PAL"))->readAll();
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
		col.a = SDL_ALPHA_OPAQUE;
		startPoint++;
		playerColorPalette[i] = col;
	}

	neutralColorPalette = new SDL_Color[32];

	auto stream = CResourceHandler::get()->load(ResourceID("config/NEUTRAL.PAL"));
	CBinaryReader reader(stream.get());

	for(int i=0; i<32; ++i)
	{
		neutralColorPalette[i].r = reader.readUInt8();
		neutralColorPalette[i].g = reader.readUInt8();
		neutralColorPalette[i].b = reader.readUInt8();
		reader.readUInt8(); // this is "flags" entry, not alpha
		neutralColorPalette[i].a = SDL_ALPHA_OPAQUE;
	}
	//colors initialization
	SDL_Color colors[]  = { 
		{0xff,0,  0,    SDL_ALPHA_OPAQUE}, 
		{0x31,0x52,0xff,SDL_ALPHA_OPAQUE},
		{0x9c,0x73,0x52,SDL_ALPHA_OPAQUE},
		{0x42,0x94,0x29,SDL_ALPHA_OPAQUE},
		
		{0xff,0x84,0,   SDL_ALPHA_OPAQUE},
		{0x8c,0x29,0xa5,SDL_ALPHA_OPAQUE},
		{0x09,0x9c,0xa5,SDL_ALPHA_OPAQUE},
		{0xc6,0x7b,0x8c,SDL_ALPHA_OPAQUE}};		
		
	for(int i=0;i<8;i++)
	{
		playerColors[i] = colors[i];
	}
	//gray
	neutralColor->r = 0x84;
	neutralColor->g = 0x84;
	neutralColor->b = 0x84;
	neutralColor->a = SDL_ALPHA_OPAQUE;
}

void Graphics::initializeBattleGraphics()
{
	const JsonNode config(ResourceID("config/battles_graphics.json"));
	
	// Reserve enough space for the terrains
	int idx = config["backgrounds"].Vector().size();
	battleBacks.resize(idx+1);	// 1 to idx, 0 is unused

	idx = 1;
	for(const JsonNode &t : config["backgrounds"].Vector()) {
		battleBacks[idx].push_back(t.String());
		idx++;
	}

	//initialization of AC->def name mapping
	for(const JsonNode &ac : config["ac_mapping"].Vector()) {
		int ACid = ac["id"].Float();
		std::vector< std::string > toAdd;

		for(const JsonNode &defname : ac["defnames"].Vector()) {
			toAdd.push_back(defname.String());
		}

		battleACToDef[ACid] = toAdd;
	}	
}
Graphics::Graphics()
{
	#if 0
	std::vector<Task> tasks; //preparing list of graphics to load
	tasks += std::bind(&Graphics::loadFonts,this);
	tasks += std::bind(&Graphics::loadPaletteAndColors,this);
	tasks += std::bind(&Graphics::loadHeroFlags,this);
	tasks += std::bind(&Graphics::initializeBattleGraphics,this);
	tasks += std::bind(&Graphics::loadErmuToPicture,this);
	tasks += std::bind(&Graphics::initializeImageLists,this);
	tasks += GET_DEF_ESS(resources32,"RESOURCE.DEF");	
	tasks += GET_DEF_ESS(heroMoveArrows,"ADAG.DEF");

	CThreadHelper th(&tasks,std::max((ui32)1,boost::thread::hardware_concurrency()));
	th.run();
	#else
	loadFonts();
	loadPaletteAndColors();
	loadHeroFlags();
	initializeBattleGraphics();
	loadErmuToPicture();
	initializeImageLists();
	resources32 = CDefHandler::giveDefEss("RESOURCE.DEF");
	heroMoveArrows = CDefHandler::giveDefEss("ADAG.DEF");
	#endif

	for(auto & elem : heroMoveArrows->ourImages)
	{
		CSDL_Ext::alphaTransform(elem.bitmap);
	}
}

void Graphics::loadHeroAnims()
{
	//first - group number to be rotated1, second - group number after rotation1
	std::vector<std::pair<int,int> > rotations = 
	{
		{6,10}, {7,11}, {8,12}, {1,13},
		{2,14}, {3,15}
	};

	for(auto & elem : CGI->heroh->classes.heroClasses)
	{
		for (auto & templ : VLC->objtypeh->getHandlerFor(Obj::HERO, elem->id)->getTemplates())
		{
			if (!heroAnims.count(templ.animationFile))
			heroAnims[templ.animationFile] = loadHeroAnim(templ.animationFile, rotations);
		}
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
					nci.bitmap = CSDL_Ext::verticalFlip(anim->ourImages[o+e].bitmap);
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
	for(auto & elem : anim->ourImages)
	{
		CSDL_Ext::alphaTransform(elem.bitmap);
	}
	return anim;
}

void Graphics::loadHeroFlagsDetail(std::pair<std::vector<CDefEssential *> Graphics::*, std::vector<const char *> > &pr, bool mode)
{
	for(int i=0;i<8;i++)
		(this->*pr.first).push_back(CDefHandler::giveDefEss(pr.second[i]));
	//first - group number to be rotated1, second - group number after rotation1
	std::vector<std::pair<int,int> > rotations = 
	{
		{6,10}, {7,11}, {8,12}
	};
	
	for(int q=0; q<8; ++q)
	{
		std::vector<Cimage> &curImgs = (this->*pr.first)[q]->ourImages;
		for(size_t o=0; o<curImgs.size(); ++o)
		{
			for(auto & rotation : rotations)
			{
				if(curImgs[o].groupNumber==rotation.first)
				{
					for(int e=0; e<8; ++e)
					{
						Cimage nci;
						nci.bitmap = CSDL_Ext::verticalFlip(curImgs[o+e].bitmap);
						nci.groupNumber = rotation.second;
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
						nci.bitmap = CSDL_Ext::verticalFlip(curImgs[o+e].bitmap);
						nci.groupNumber = 12 + curImgs[o].groupNumber;
						nci.imName = std::string();
						curImgs.push_back(nci);
					}
					o+=8;
				}
			}
		}
		for(auto & curImg : curImgs)
		{
			CSDL_Ext::setDefaultColorKey(curImg.bitmap);
			SDL_SetSurfaceBlendMode(curImg.bitmap,SDL_BLENDMODE_NONE);
		}
	}
}

void Graphics::loadHeroFlags()
{
	CStopWatch th;
	std::pair<std::vector<CDefEssential *> Graphics::*, std::vector<const char *> > pr[4] =
	{
		{
			&Graphics::flags1, 
			{"ABF01L.DEF","ABF01G.DEF","ABF01R.DEF","ABF01D.DEF","ABF01B.DEF",
			"ABF01P.DEF","ABF01W.DEF","ABF01K.DEF"} 
		},
		{
			&Graphics::flags2,
			{"ABF02L.DEF","ABF02G.DEF","ABF02R.DEF","ABF02D.DEF","ABF02B.DEF",
			"ABF02P.DEF","ABF02W.DEF","ABF02K.DEF"}
			
		},
		{
			&Graphics::flags3,
			{"ABF03L.DEF","ABF03G.DEF","ABF03R.DEF","ABF03D.DEF","ABF03B.DEF",
			"ABF03P.DEF","ABF03W.DEF","ABF03K.DEF"}
		},
		{
			&Graphics::flags4,
			{"AF00.DEF","AF01.DEF","AF02.DEF","AF03.DEF","AF04.DEF",
			"AF05.DEF","AF06.DEF","AF07.DEF"}
		}		
	};

	#if 0
	boost::thread_group grupa;
	for(int g=3; g>=0; --g)
	{
		grupa.create_thread(std::bind(&Graphics::loadHeroFlagsDetail, this, std::ref(pr[g]), true));
	}
	grupa.join_all();
	#else
	for(auto p: pr)
	{
		loadHeroFlagsDetail(p,true);
	}
	#endif
	logGlobal->infoStream() << "Loading and transforming heroes' flags: "<<th.getDiff();
}

void Graphics::blueToPlayersAdv(SDL_Surface * sur, PlayerColor player)
{
	if(sur->format->palette)
	{
		SDL_Color *palette = nullptr;
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
	}
	else
	{
		//TODO: implement. H3 method works only for images with palettes.
		// Add some kind of player-colored overlay?
		// Or keep palette approach here and replace only colors of specific value(s)
		// Or just wait for OpenGL support?
		logGlobal->warnStream() << "Image must have palette to be player-colored!";
	}
}

void Graphics::loadFonts()
{
	const JsonNode config(ResourceID("config/fonts.json"));

	const JsonVector & bmpConf = config["bitmap"].Vector();
	const JsonNode   & ttfConf = config["trueType"];
	const JsonNode   & hanConf = config["bitmapHan"];

	assert(bmpConf.size() == FONTS_NUMBER);

	for (size_t i=0; i<FONTS_NUMBER; i++)
	{
		std::string filename = bmpConf[i].String();

		if (!hanConf[filename].isNull())
			fonts[i] = new CBitmapHanFont(hanConf[filename]);
		else if (!ttfConf[filename].isNull()) // no ttf override
			fonts[i] = new CTrueTypeFont(ttfConf[filename]);
		else
			fonts[i] = new CBitmapFont(filename);
	}
}

CDefEssential * Graphics::getDef( const CGObjectInstance * obj )
{
	if (obj->appearance.animationFile.empty())
	{
		logGlobal->warnStream() << boost::format("Def name for obj %d (%d,%d) is empty!") % obj->id % obj->ID % obj->subID;
		return nullptr;
	}
	return advmapobjGraphics[obj->appearance.animationFile];
}

CDefEssential * Graphics::getDef( const ObjectTemplate & info )
{
	if (info.animationFile.empty())
	{
		logGlobal->warnStream() << boost::format("Def name for obj (%d,%d) is empty!") % info.id % info.subid;
		return nullptr;
	}
	return advmapobjGraphics[info.animationFile];
}

void Graphics::loadErmuToPicture()
{
	//loading ERMU to picture
	const JsonNode config(ResourceID("config/ERMU_to_picture.json"));
	int etp_idx = 0;
	for(const JsonNode &etp : config["ERMU_to_picture"].Vector()) {
		int idx = 0;
		for(const JsonNode &n : etp.Vector()) {
			ERMUtoPicture[idx][etp_idx] = n.String();
			idx ++;
		}
		assert (idx == ARRAY_COUNT(ERMUtoPicture));

		etp_idx ++;
	}
	assert (etp_idx == 44);
}

void Graphics::addImageListEntry(size_t index, std::string listName, std::string imageName)
{
	if (!imageName.empty())
	{
		JsonNode entry;
		entry["frame"].Float() = index;
		entry["file"].String() = imageName;

		imageLists["SPRITES/" + listName]["images"].Vector().push_back(entry);
	}
}

void Graphics::initializeImageLists()
{
	for(const CCreature * creature : CGI->creh->creatures)
	{
		addImageListEntry(creature->iconIndex, "CPRSMALL", creature->smallIconName);
		addImageListEntry(creature->iconIndex, "TWCRPORT", creature->largeIconName);
	}

	for(const CHero * hero : CGI->heroh->heroes)
	{
		addImageListEntry(hero->imageIndex, "UN32", hero->iconSpecSmall);
		addImageListEntry(hero->imageIndex, "UN44", hero->iconSpecLarge);
		addImageListEntry(hero->imageIndex, "PORTRAITSLARGE", hero->portraitLarge);
		addImageListEntry(hero->imageIndex, "PORTRAITSSMALL", hero->portraitSmall);
	}

	for(const CArtifact * art : CGI->arth->artifacts)
	{
		addImageListEntry(art->iconIndex, "ARTIFACT", art->image);
		addImageListEntry(art->iconIndex, "ARTIFACTLARGE", art->large);
	}

	for(const CFaction * faction : CGI->townh->factions)
	{
		if (faction->town)
		{
			auto & info = faction->town->clientInfo;
			addImageListEntry(info.icons[0][0], "ITPT", info.iconLarge[0][0]);
			addImageListEntry(info.icons[0][1], "ITPT", info.iconLarge[0][1]);
			addImageListEntry(info.icons[1][0], "ITPT", info.iconLarge[1][0]);
			addImageListEntry(info.icons[1][1], "ITPT", info.iconLarge[1][1]);

			addImageListEntry(info.icons[0][0] + 2, "ITPA", info.iconSmall[0][0]);
			addImageListEntry(info.icons[0][1] + 2, "ITPA", info.iconSmall[0][1]);
			addImageListEntry(info.icons[1][0] + 2, "ITPA", info.iconSmall[1][0]);
			addImageListEntry(info.icons[1][1] + 2, "ITPA", info.iconSmall[1][1]);
		}
	}
	
	for(const CSpell * spell : CGI->spellh->objects)
	{
		addImageListEntry(spell->id, "SPELLS", spell->iconBook);
		addImageListEntry(spell->id+1, "SPELLINT", spell->iconEffect);
		addImageListEntry(spell->id, "SPELLBON", spell->iconScenarioBonus);
		addImageListEntry(spell->id, "SPELLSCR", spell->iconScroll);		
	}
}
