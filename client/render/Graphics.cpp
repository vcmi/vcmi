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

#include <vcmi/Entity.h>
#include <vcmi/ArtifactService.h>
#include <vcmi/CreatureService.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/SkillService.h>
#include <vcmi/spells/Service.h>

#include "../renderSDL/SDL_Extensions.h"
#include "../renderSDL/CBitmapFont.h"
#include "../renderSDL/CBitmapHanFont.h"
#include "../renderSDL/CTrueTypeFont.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../gui/CGuiHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CBinaryReader.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModScope.h"
#include "CGameInfo.h"
#include "../lib/VCMI_Lib.h"
#include "../CCallback.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/JsonNode.h"
#include "../lib/vcmi_endian.h"
#include "../lib/CStopWatch.h"
#include "../lib/CHeroHandler.h"

#include <SDL_surface.h>

Graphics * graphics = nullptr;

void Graphics::loadPaletteAndColors()
{
	auto textFile = CResourceHandler::get()->load(ResourcePath("DATA/PLAYERS.PAL"))->readAll();
	std::string pals((char*)textFile.first.get(), textFile.second);

	int startPoint = 24; //beginning byte; used to read
	for(int i=0; i<8; ++i)
	{
		for(int j=0; j<32; ++j)
		{
			ColorRGBA col;
			col.r = pals[startPoint++];
			col.g = pals[startPoint++];
			col.b = pals[startPoint++];
			col.a = SDL_ALPHA_OPAQUE;
			startPoint++;
			playerColorPalette[i][j] = col;
		}
	}

	auto stream = CResourceHandler::get()->load(ResourcePath("config/NEUTRAL.PAL"));
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
	ColorRGBA colors[]  = {
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
	neutralColor.r = 0x84;
	neutralColor.g = 0x84;
	neutralColor.b = 0x84;
	neutralColor.a = SDL_ALPHA_OPAQUE;
}

void Graphics::initializeBattleGraphics()
{
	auto allConfigs = VLC->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), ModScope::scopeBuiltin());
	for(auto & mod : allConfigs)
	{
		if(!CResourceHandler::get(mod)->existsResource(ResourcePath("config/battles_graphics.json")))
			continue;
			
		const JsonNode config(mod, JsonPath::builtin("config/battles_graphics.json"));

		//initialization of AC->def name mapping
		if(!config["ac_mapping"].isNull())
		for(const JsonNode &ac : config["ac_mapping"].Vector())
		{
			int ACid = static_cast<int>(ac["id"].Float());
			std::vector< std::string > toAdd;

			for(const JsonNode &defname : ac["defnames"].Vector())
			{
				toAdd.push_back(defname.String());
			}

			battleACToDef[ACid] = toAdd;
		}
	}
}
Graphics::Graphics()
{
	loadFonts();
	loadPaletteAndColors();
	initializeBattleGraphics();
	loadErmuToPicture();
	initializeImageLists();

	//(!) do not load any CAnimation here
}

void Graphics::blueToPlayersAdv(SDL_Surface * sur, PlayerColor player)
{
	if(sur->format->palette)
	{
		SDL_Color palette[32];
		if(player.isValidPlayer())
		{
			for(int i=0; i<32; ++i)
				palette[i] = CSDL_Ext::toSDL(playerColorPalette[player][i]);
		}
		else if(player == PlayerColor::NEUTRAL)
		{
			for(int i=0; i<32; ++i)
				palette[i] = CSDL_Ext::toSDL(neutralColorPalette[i]);
		}
		else
		{
			logGlobal->error("Wrong player id in blueToPlayersAdv (%s)!", player.toString());
			return;
		}
//FIXME: not all player colored images have player palette at last 32 indexes
//NOTE: following code is much more correct but still not perfect (bugged with status bar)
		CSDL_Ext::setColors(sur, palette, 224, 32);


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

void Graphics::loadFonts()
{
	const JsonNode config(JsonPath::builtin("config/fonts.json"));

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

void Graphics::loadErmuToPicture()
{
	//loading ERMU to picture
	const JsonNode config(JsonPath::builtin("config/ERMU_to_picture.json"));
	int etp_idx = 0;
	for(const JsonNode &etp : config["ERMU_to_picture"].Vector()) {
		int idx = 0;
		for(const JsonNode &n : etp.Vector()) {
			ERMUtoPicture[idx][etp_idx] = n.String();
			idx ++;
		}
		assert (idx == std::size(ERMUtoPicture));

		etp_idx ++;
	}
	assert (etp_idx == 44);
}

void Graphics::addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName)
{
	if (!imageName.empty())
	{
		JsonNode entry;
		if (group != 0)
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
	addImageListEntries(CGI->creatures());
	addImageListEntries(CGI->heroTypes());
	addImageListEntries(CGI->artifacts());
	addImageListEntries(CGI->factions());
	addImageListEntries(CGI->spells());
	addImageListEntries(CGI->skills());
}

std::shared_ptr<CAnimation> Graphics::getAnimation(const AnimationPath & path)
{
	if (cachedAnimations.count(path) != 0)
		return cachedAnimations.at(path);

	auto newAnimation = GH.renderHandler().loadAnimation(path);

	newAnimation->preload();
	cachedAnimations[path] = newAnimation;
	return newAnimation;
}
