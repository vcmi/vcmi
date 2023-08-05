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

#include "IFont.h"
#include "../lib/GameConstants.h"

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
struct SDL_Color;
class CAnimation;

enum EFonts : int
{
	FONT_BIG, FONT_CALLI, FONT_CREDITS, FONT_HIGH_SCORE, FONT_MEDIUM, FONT_SMALL, FONT_TIMES, FONT_TINY, FONT_VERD
};

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

	std::map<std::string, std::shared_ptr<CAnimation>> cachedAnimations;

public:
	std::shared_ptr<CAnimation> getAnimation(const std::string & path);

	//Fonts
	static const int FONTS_NUMBER = 9;
	std::array< std::shared_ptr<IFont>, FONTS_NUMBER> fonts;

	//various graphics
	SDL_Color * playerColors; //array [8]
	SDL_Color * neutralColor;
	SDL_Color * playerColorPalette; //palette to make interface colors good - array of size [256]
	SDL_Color * neutralColorPalette;

	std::map<std::string, JsonNode> imageLists;

	//towns
	std::map<int, std::string> ERMUtoPicture[GameConstants::F_NUMBER]; //maps building ID to it's picture's name for each town type
	//for battles
	std::map< int, std::vector < std::string > > battleACToDef; //maps AC format to vector of appropriate def names

	//functions
	Graphics();
	~Graphics();

	void blueToPlayersAdv(SDL_Surface * sur, PlayerColor player); //replaces blue interface colour with a color of player
};

extern Graphics * graphics;
