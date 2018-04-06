/*
 * Graphics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Graphics.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CBinaryReader.h"
#include "gui/SDL_Extensions.h"
#include "gui/CAnimation.h"
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
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CGameState.h"
#include "../lib/JsonNode.h"
#include "../lib/vcmi_endian.h"
#include "../lib/CStopWatch.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/mapObjects/CObjectHandler.h"

using namespace CSDL_Ext;

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
	tasks += std::bind(&Graphics::initializeBattleGraphics,this);
	tasks += std::bind(&Graphics::loadErmuToPicture,this);
	tasks += std::bind(&Graphics::initializeImageLists,this);

	CThreadHelper th(&tasks,std::max((ui32)1,boost::thread::hardware_concurrency()));
	th.run();
	#else
	loadFonts();
	loadPaletteAndColors();
	initializeBattleGraphics();
	loadErmuToPicture();
	initializeImageLists();
	#endif

	//(!) do not load any CAnimation here
}

Graphics::~Graphics()
{
	delete[] playerColors;
	delete neutralColor;
	delete[] playerColorPalette;
	delete[] neutralColorPalette;
}

void Graphics::load()
{
	heroMoveArrows = std::make_shared<CAnimation>("ADAG");
	heroMoveArrows->preload();

	loadHeroAnimations();
	loadHeroFlagAnimations();
	loadFogOfWar();
}

void Graphics::loadHeroAnimations()
{
	for(auto & elem : CGI->heroh->classes.heroClasses)
	{
		for (auto & templ : VLC->objtypeh->getHandlerFor(Obj::HERO, elem->id)->getTemplates())
		{
			if (!heroAnimations.count(templ.animationFile))
				heroAnimations[templ.animationFile] = loadHeroAnimation(templ.animationFile);
		}
	}

	boatAnimations[0] = loadHeroAnimation("AB01_.DEF");
	boatAnimations[1] = loadHeroAnimation("AB02_.DEF");
	boatAnimations[2] = loadHeroAnimation("AB03_.DEF");


	mapObjectAnimations["AB01_.DEF"] = boatAnimations[0];
	mapObjectAnimations["AB02_.DEF"] = boatAnimations[1];
	mapObjectAnimations["AB03_.DEF"] = boatAnimations[2];
}
void Graphics::loadHeroFlagAnimations()
{
	static const std::vector<std::string> HERO_FLAG_ANIMATIONS =
	{
		"AF00", "AF01","AF02","AF03",
		"AF04", "AF05","AF06","AF07"
	};

	static const std::vector< std::vector<std::string> > BOAT_FLAG_ANIMATIONS =
	{
		{
			"ABF01L", "ABF01G", "ABF01R", "ABF01D",
			"ABF01B", "ABF01P", "ABF01W", "ABF01K"
		},
		{
			"ABF02L", "ABF02G", "ABF02R", "ABF02D",
			"ABF02B", "ABF02P", "ABF02W", "ABF02K"
		},
		{
			"ABF03L", "ABF03G", "ABF03R", "ABF03D",
			"ABF03B", "ABF03P", "ABF03W", "ABF03K"
		}
	};

	for(const auto & name : HERO_FLAG_ANIMATIONS)
		heroFlagAnimations.push_back(loadHeroFlagAnimation(name));

	for(int i = 0; i < BOAT_FLAG_ANIMATIONS.size(); i++)
		for(const auto & name : BOAT_FLAG_ANIMATIONS[i])
			boatFlagAnimations[i].push_back(loadHeroFlagAnimation(name));
}

std::shared_ptr<CAnimation> Graphics::loadHeroFlagAnimation(const std::string & name)
{
	//first - group number to be rotated, second - group number after rotation
	static const std::vector<std::pair<int,int> > rotations =
	{
		{6,10}, {7,11}, {8,12}, {1,13},
		{2,14}, {3,15}
	};

	std::shared_ptr<CAnimation> anim = std::make_shared<CAnimation>(name);
	anim->preload();

	for(const auto & rotation : rotations)
	{
		const int sourceGroup = rotation.first;
		const int targetGroup = rotation.second;

		anim->createFlippedGroup(sourceGroup, targetGroup);
	}

	return anim;
}

std::shared_ptr<CAnimation> Graphics::loadHeroAnimation(const std::string &name)
{
	//first - group number to be rotated, second - group number after rotation
	static const std::vector<std::pair<int,int> > rotations =
	{
		{6,10}, {7,11}, {8,12}, {1,13},
		{2,14}, {3,15}
	};

	std::shared_ptr<CAnimation> anim = std::make_shared<CAnimation>(name);
	anim->preload();


	for(const auto & rotation : rotations)
	{
		const int sourceGroup = rotation.first;
		const int targetGroup = rotation.second;

		anim->createFlippedGroup(sourceGroup, targetGroup);
	}

	return anim;
}

void Graphics::blueToPlayersAdv(SDL_Surface * sur, PlayerColor player)
{
	if(sur->format->palette)
	{
		SDL_Color * palette = nullptr;
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
			logGlobal->error("Wrong player id in blueToPlayersAdv (%s)!", player.getStr());
			return;
		}
//FIXME: not all player colored images have player palette at last 32 indexes
//NOTE: following code is much more correct but still not perfect (bugged with status bar)

		SDL_SetColors(sur, palette, 224, 32);


#if 0

		SDL_Color * bluePalette = playerColorPalette + 32;

		SDL_Palette * oldPalette = sur->format->palette;

		SDL_Palette * newPalette = SDL_AllocPalette(256);

		for(size_t destIndex = 0; destIndex < 256; destIndex++)
		{
			SDL_Color old = oldPalette->colors[destIndex];

			bool found = false;

			for(size_t srcIndex = 0; srcIndex < 32; srcIndex++)
			{
				if(old.b == bluePalette[srcIndex].b && old.g == bluePalette[srcIndex].g && old.r == bluePalette[srcIndex].r)
				{
					found = true;
					newPalette->colors[destIndex] = palette[srcIndex];
					break;
				}
			}
			if(!found)
				newPalette->colors[destIndex] = old;
		}

		SDL_SetSurfacePalette(sur, newPalette);

		SDL_FreePalette(newPalette);

#endif // 0
	}
	else
	{
		//TODO: implement. H3 method works only for images with palettes.
		// Add some kind of player-colored overlay?
		// Or keep palette approach here and replace only colors of specific value(s)
		// Or just wait for OpenGL support?
		logGlobal->warn("Image must have palette to be player-colored!");
	}
}

void Graphics::loadFogOfWar()
{
	fogOfWarFullHide = std::make_shared<CAnimation>("TSHRC");
	fogOfWarFullHide->preload();
	fogOfWarPartialHide = std::make_shared<CAnimation>("TSHRE");
	fogOfWarPartialHide->preload();

	static const int rotations [] = {22, 15, 2, 13, 12, 16, 28, 17, 20, 19, 7, 24, 26, 25, 30, 32, 27};

	size_t size = fogOfWarPartialHide->size(0);//group size after next rotation

	for(const int rotation : rotations)
	{
		fogOfWarPartialHide->duplicateImage(0, rotation, 0);
		auto image = fogOfWarPartialHide->getImage(size, 0);
		image->verticalFlip();
		size++;
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
			fonts[i] = std::make_shared<CBitmapHanFont>(hanConf[filename]);
		else if (!ttfConf[filename].isNull()) // no ttf override
			fonts[i] = std::make_shared<CTrueTypeFont>(ttfConf[filename]);
		else
			fonts[i] = std::make_shared<CBitmapFont>(filename);
	}
}

std::shared_ptr<CAnimation> Graphics::getAnimation(const CGObjectInstance* obj)
{
	return getAnimation(obj->appearance);
}

std::shared_ptr<CAnimation> Graphics::getAnimation(const ObjectTemplate & info)
{
	//the only(?) invisible object
	if(info.id == Obj::EVENT)
	{
		return std::shared_ptr<CAnimation>();
	}

	if(info.animationFile.empty())
	{
		logGlobal->warn("Def name for obj (%d,%d) is empty!", info.id, info.subid);
		return std::shared_ptr<CAnimation>();
	}

	std::shared_ptr<CAnimation> ret = mapObjectAnimations[info.animationFile];

	//already loaded
	if(ret)
	{
		ret->preload();
		return ret;
	}

	ret = std::make_shared<CAnimation>(info.animationFile);
	mapObjectAnimations[info.animationFile] = ret;

	ret->preload();
	return ret;
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

	for(const CSkill * skill : CGI->skillh->objects)
	{
		for(int level = 1; level <= 3; level++)
		{
			int frame = 2 + level + 3 * skill->id;
			const CSkill::LevelInfo & skillAtLevel = skill->at(level);
			addImageListEntry(frame, "SECSK32", skillAtLevel.iconSmall);
			addImageListEntry(frame, "SECSKILL", skillAtLevel.iconMedium);
			addImageListEntry(frame, "SECSK82", skillAtLevel.iconLarge);
		}
	}
}
