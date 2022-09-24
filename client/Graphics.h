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

#include "gui/Fonts.h"
#include "../lib/GameConstants.h"
#include "gui/Geometries.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
class CHeroClass;
struct InfoAboutHero;
struct InfoAboutTown;
class CGObjectInstance;
class ObjectTemplate;
class EntityService;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;
class CAnimation;

enum EFonts
{
	FONT_BIG, FONT_CALLI, FONT_CREDITS, FONT_HIGH_SCORE, FONT_MEDIUM, FONT_SMALL, FONT_TIMES, FONT_TINY, FONT_VERD
};

/// Handles fonts, hero images, town images, various graphics
class Graphics
{
	void addImageListEntry(size_t index, const std::string & listName, const std::string & imageName);

	void addImageListEntries(const EntityService * service);

	void initializeBattleGraphics();
	void loadPaletteAndColors();

	void loadHeroAnimations();
	//loads animation and adds required rotated frames
	std::shared_ptr<CAnimation> loadHeroAnimation(const std::string &name);

	void loadHeroFlagAnimations();

	//loads animation and adds required rotated frames
	std::shared_ptr<CAnimation> loadHeroFlagAnimation(const std::string &name);

	void loadErmuToPicture();
	void loadFogOfWar();
	void loadFonts();
	void initializeImageLists();

public:
	//Fonts
	static const int FONTS_NUMBER = 9;
	std::array< std::shared_ptr<IFont>, FONTS_NUMBER> fonts;

	//various graphics
	SDL_Color * playerColors; //array [8]
	SDL_Color * neutralColor;
	SDL_Color * playerColorPalette; //palette to make interface colors good - array of size [256]
	SDL_Color * neutralColorPalette;

	std::shared_ptr<CAnimation> heroMoveArrows;

	// [hero class def name]  //added group 10: up - left, 11 - left and 12 - left down // 13 - up-left standing; 14 - left standing; 15 - left down standing
	std::map< std::string, std::shared_ptr<CAnimation> > heroAnimations;
	std::vector< std::shared_ptr<CAnimation> > heroFlagAnimations;

	// [boat type: 0 .. 2]  //added group 10: up - left, 11 - left and 12 - left down // 13 - up-left standing; 14 - left standing; 15 - left down standing
	std::array< std::shared_ptr<CAnimation>, 3> boatAnimations;

	std::array< std::vector<std::shared_ptr<CAnimation> >, 3> boatFlagAnimations;

	//all other objects (not hero or boat)
	std::map< std::string, std::shared_ptr<CAnimation> > mapObjectAnimations;

	std::shared_ptr<CAnimation> fogOfWarFullHide;
	std::shared_ptr<CAnimation> fogOfWarPartialHide;

	std::map<std::string, JsonNode> imageLists;

	//towns
	std::map<int, std::string> ERMUtoPicture[GameConstants::F_NUMBER]; //maps building ID to it's picture's name for each town type
	//for battles
	std::map< int, std::vector < std::string > > battleACToDef; //maps AC format to vector of appropriate def names

	//functions
	Graphics();
	~Graphics();

	void load();

	void blueToPlayersAdv(SDL_Surface * sur, PlayerColor player); //replaces blue interface colour with a color of player

	std::shared_ptr<CAnimation> getAnimation(const CGObjectInstance * obj);
	std::shared_ptr<CAnimation> getAnimation(std::shared_ptr<const ObjectTemplate> info);
};

extern Graphics * graphics;
