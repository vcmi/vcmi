/*
 * Graphics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/GameConstants.h"
#include "../lib/Color.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
class CHeroClass;
struct InfoAboutHero;
struct InfoAboutTown;
class CGObjectInstance;
class ObjectTemplate;
class EntityService;
class JsonNode;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class CAnimation;
class IFont;

/// Handles fonts, hero images, town images, various graphics
class Graphics
{
	void addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName);
	void addImageListEntries(const EntityService * service);

	void initializeBattleGraphics();
	void loadPaletteAndColors();
	void loadErmuToPicture();
	void loadFonts();
	void initializeImageLists();

public:
	//Fonts
	static const int FONTS_NUMBER = 9;
	std::array< std::shared_ptr<IFont>, FONTS_NUMBER> fonts;

	using PlayerPalette = std::array<ColorRGBA, 32>;

	//various graphics
	std::array<ColorRGBA, 8> playerColors;
	std::array<PlayerPalette, 8> playerColorPalette; //palette to make interface colors good - array of size [256]

	PlayerPalette neutralColorPalette;
	ColorRGBA neutralColor;

	std::map<std::string, JsonNode> imageLists;

	//towns
	std::map<int, std::string> ERMUtoPicture[GameConstants::F_NUMBER]; //maps building ID to it's picture's name for each town type
	//for battles
	std::map< int, std::vector < std::string > > battleACToDef; //maps AC format to vector of appropriate def names

	//functions
	Graphics();

	void blueToPlayersAdv(SDL_Surface * sur, PlayerColor player); //replaces blue interface colour with a color of player
};

extern Graphics * graphics;
