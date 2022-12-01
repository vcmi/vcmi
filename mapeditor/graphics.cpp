/*
 * graphics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//code is copied from vcmiclient/Graphics.cpp with minimal changes
#include "StdInc.h"
#include "graphics.h"

#include <vcmi/Entity.h>
#include <vcmi/ArtifactService.h>
#include <vcmi/CreatureService.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/SkillService.h>
#include <vcmi/spells/Service.h>

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CBinaryReader.h"
#include "Animation.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CModHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../CCallback.h"
#include "../lib/CGeneralTextHandler.h"
#include "BitmapHandler.h"
#include "../lib/CGameState.h"
#include "../lib/JsonNode.h"
#include "../lib/CStopWatch.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"
#include "../lib/mapObjects/CObjectHandler.h"
#include "../lib/CHeroHandler.h"

Graphics * graphics = nullptr;

void Graphics::loadPaletteAndColors()
{
	auto textFile = CResourceHandler::get()->load(ResourceID("DATA/PLAYERS.PAL"))->readAll();
	std::string pals((char*)textFile.first.get(), textFile.second);
	
	playerColorPalette.resize(256);
	playerColors.resize(PlayerColor::PLAYER_LIMIT_I);
	int startPoint = 24; //beginning byte; used to read
	for(int i = 0; i < 256; ++i)
	{
		QColor col;
		col.setRed(pals[startPoint++]);
		col.setGreen(pals[startPoint++]);
		col.setBlue(pals[startPoint++]);
		col.setAlpha(255);
		startPoint++;
		playerColorPalette[i] = col.rgba();
	}
	
	neutralColorPalette.resize(32);
	
	auto stream = CResourceHandler::get()->load(ResourceID("config/NEUTRAL.PAL"));
	CBinaryReader reader(stream.get());
	
	for(int i = 0; i < 32; ++i)
	{
		QColor col;
		col.setRed(reader.readUInt8());
		col.setGreen(reader.readUInt8());
		col.setBlue(reader.readUInt8());
		col.setAlpha(255);
		reader.readUInt8(); // this is "flags" entry, not alpha
		neutralColorPalette[i] = col.rgba();
	}
	
	//colors initialization
	QColor colors[]  = {
		{0xff,0,  0,    255},
		{0x31,0x52,0xff,255},
		{0x9c,0x73,0x52,255},
		{0x42,0x94,0x29,255},
		
		{0xff,0x84,0,   255},
		{0x8c,0x29,0xa5,255},
		{0x09,0x9c,0xa5,255},
		{0xc6,0x7b,0x8c,255}};
	
	for(int i=0;i<8;i++)
	{
		playerColors[i] = colors[i].rgba();
	}
	//gray
	neutralColor = qRgba(0x84, 0x84, 0x84, 0xFF);
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
	loadPaletteAndColors();
	initializeImageLists();
#endif
	
	//(!) do not load any CAnimation here
}

Graphics::~Graphics()
{
}

void Graphics::load()
{
	loadHeroAnimations();
	loadHeroFlagAnimations();
}

void Graphics::loadHeroAnimations()
{
	for(auto & elem : VLC->heroh->classes.objects)
	{
		for(auto templ : VLC->objtypeh->getHandlerFor(Obj::HERO, elem->getIndex())->getTemplates())
		{
			if(!heroAnimations.count(templ->animationFile))
				heroAnimations[templ->animationFile] = loadHeroAnimation(templ->animationFile);
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

std::shared_ptr<Animation> Graphics::loadHeroFlagAnimation(const std::string & name)
{
	//first - group number to be rotated, second - group number after rotation
	static const std::vector<std::pair<int,int> > rotations =
	{
		{6,10}, {7,11}, {8,12}, {1,13},
		{2,14}, {3,15}
	};
	
	std::shared_ptr<Animation> anim = std::make_shared<Animation>(name);
	anim->preload();
	
	for(const auto & rotation : rotations)
	{
		const int sourceGroup = rotation.first;
		const int targetGroup = rotation.second;
		
		anim->createFlippedGroup(sourceGroup, targetGroup);
	}
	
	return anim;
}

std::shared_ptr<Animation> Graphics::loadHeroAnimation(const std::string &name)
{
	//first - group number to be rotated, second - group number after rotation
	static const std::vector<std::pair<int,int> > rotations =
	{
		{6,10}, {7,11}, {8,12}, {1,13},
		{2,14}, {3,15}
	};
	
	std::shared_ptr<Animation> anim = std::make_shared<Animation>(name);
	anim->preload();
	
	
	for(const auto & rotation : rotations)
	{
		const int sourceGroup = rotation.first;
		const int targetGroup = rotation.second;
		
		anim->createFlippedGroup(sourceGroup, targetGroup);
	}
	
	return anim;
}

void Graphics::blueToPlayersAdv(QImage * sur, PlayerColor player)
{
	if(sur->format() == QImage::Format_Indexed8)
	{
		auto palette = sur->colorTable();
		if(player < PlayerColor::PLAYER_LIMIT)
		{
			for(int i = 0; i < 32; ++i)
				palette[224 + i] = playerColorPalette[player.getNum() * 32 + i];
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
		sur->setColorTable(palette);
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

std::shared_ptr<Animation> Graphics::getAnimation(const CGObjectInstance* obj)
{
	if(obj->ID == Obj::HERO)
		return getHeroAnimation(obj->appearance);
	return getAnimation(obj->appearance);
}

std::shared_ptr<Animation> Graphics::getHeroAnimation(const std::shared_ptr<const ObjectTemplate> info)
{
	if(info->animationFile.empty())
	{
		logGlobal->warn("Def name for hero (%d,%d) is empty!", info->id, info->subid);
		return std::shared_ptr<Animation>();
	}
	
	std::shared_ptr<Animation> ret = loadHeroAnimation(info->animationFile);
	
	//already loaded
	if(ret)
	{
		ret->preload();
		return ret;
	}
	
	ret = std::make_shared<Animation>(info->animationFile);
	heroAnimations[info->animationFile] = ret;
	
	ret->preload();
	return ret;
}

std::shared_ptr<Animation> Graphics::getAnimation(const std::shared_ptr<const ObjectTemplate> info)
{	
	if(info->animationFile.empty())
	{
		logGlobal->warn("Def name for obj (%d,%d) is empty!", info->id, info->subid);
		return std::shared_ptr<Animation>();
	}
	
	std::shared_ptr<Animation> ret = mapObjectAnimations[info->animationFile];
	
	//already loaded
	if(ret)
	{
		ret->preload();
		return ret;
	}
	
	ret = std::make_shared<Animation>(info->animationFile);
	mapObjectAnimations[info->animationFile] = ret;
	
	ret->preload();
	return ret;
}

void Graphics::addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName)
{
	if (!imageName.empty())
	{
		JsonNode entry;
		if(group != 0)
			entry["group"].Integer() = group;
		entry["frame"].Integer() = index;
		entry["file"].String() = imageName;
		
		imageLists["SPRITES/" + listName]["images"].Vector().push_back(entry);
	}
}

void Graphics::addImageListEntries(const EntityService * service)
{
	auto cb = std::bind(&Graphics::addImageListEntry, this, _1, _2, _3, _4);
	
	auto loopCb = [&](const Entity * entity, bool & stop)
	{
		entity->registerIcons(cb);
	};
	
	service->forEachBase(loopCb);
}

void Graphics::initializeImageLists()
{
	addImageListEntries(VLC->creatures());
	addImageListEntries(VLC->heroTypes());
	addImageListEntries(VLC->artifacts());
	addImageListEntries(VLC->factions());
	addImageListEntries(VLC->spells());
	addImageListEntries(VLC->skills());
}
