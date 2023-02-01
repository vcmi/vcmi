/*
 * AdventureMapClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObjectLists.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CGGarrison;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
struct Component;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;

VCMI_LIB_NAMESPACE_END

class CAnimation;
class CAnimImage;
class CShowableAnim;
class CFilledTexture;
class CButton;
class CComponent;
class CHeroTooltip;
class CTownTooltip;
class CTextBox;
class IImage;

class CMinimap;

class CMinimapInstance : public CIntObject
{
	CMinimap * parent;
	SDL_Surface * minimap;
	int level;

	//get color of selected tile on minimap
	const SDL_Color & getTileColor(const int3 & pos);

	void blitTileWithColor(const SDL_Color & color, const int3 & pos, SDL_Surface * to, int x, int y);

	//draw minimap already scaled.
	//result is not antialiased. Will result in "missing" pixels on huge maps (>144)
	void drawScaled(int level);
public:
	CMinimapInstance(CMinimap * parent, int level);
	~CMinimapInstance();

	void showAll(SDL_Surface * to) override;
	void tileToPixels (const int3 & tile, int & x, int & y, int toX = 0, int toY = 0);
	void refreshTile(const int3 & pos);
};

/// Minimap which is displayed at the right upper corner of adventure map
class CMinimap : public CIntObject
{
protected:
	std::shared_ptr<CPicture> aiShield; //the graphic displayed during AI turn
	std::shared_ptr<CMinimapInstance> minimap;
	int level;

	//to initialize colors
	std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > loadColors();

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void hover (bool on) override;
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override;

	void moveAdvMapSelection();

public:
	// terrainID -> (normal color, blocked color)
	const std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > colors;

	CMinimap(const Rect & position);

	//should be called to invalidate whole map - different player or level
	int3 translateMousePosition();
	void update();
	void setLevel(int level);
	void setAIRadar(bool on);

	void showAll(SDL_Surface * to) override;

	void hideTile(const int3 &pos); //puts FoW
	void showTile(const int3 &pos); //removes FoW
};

