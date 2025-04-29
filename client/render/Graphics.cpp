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
#include "../render/CAnimation.h"
#include "../render/IImage.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CBinaryReader.h"
#include "../../lib/json/JsonNode.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModScope.h"
#include "../lib/GameLibrary.h"

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
	auto allConfigs = LIBRARY->modh->getActiveMods();
	allConfigs.insert(allConfigs.begin(), ModScope::scopeBuiltin());
	for(auto & mod : allConfigs)
	{
		if(!CResourceHandler::get(mod)->existsResource(ResourcePath("config/battles_graphics.json")))
			continue;
			
		const JsonNode config(JsonPath::builtin("config/battles_graphics.json"), mod);

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
	loadPaletteAndColors();
	initializeBattleGraphics();
	loadErmuToPicture();

	//(!) do not load any CAnimation here
}

void Graphics::setPlayerPalette(SDL_Palette * targetPalette, PlayerColor player)
{
	SDL_Color palette[32];
	if(player.isValidPlayer())
	{
		for(int i=0; i<32; ++i)
			palette[i] = CSDL_Ext::toSDL(playerColorPalette[player.getNum()][i]);
	}
	else
	{
		for(int i=0; i<32; ++i)
			palette[i] = CSDL_Ext::toSDL(neutralColorPalette[i]);
	}

	SDL_SetPaletteColors(targetPalette, palette, 224, 32);
}

void Graphics::setPlayerFlagColor(SDL_Palette * targetPalette, PlayerColor player)
{
	if(player.isValidPlayer())
	{
		SDL_Color color = CSDL_Ext::toSDL(playerColors[player.getNum()]);
		SDL_SetPaletteColors(targetPalette, &color, 5, 1);
	}
	else
	{
		SDL_Color color = CSDL_Ext::toSDL(neutralColor);
		SDL_SetPaletteColors(targetPalette, &color, 5, 1);
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
