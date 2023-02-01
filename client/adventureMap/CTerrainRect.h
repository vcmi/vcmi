/*
 * CAdvmapInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/AdventureMapClasses.h"
#include "CWindowObject.h"

#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"

#include "../../lib/spells/ViewSpellInt.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CGPath;
struct CGPathNode;
class CGHeroInstance;
class CGTownInstance;
class CSpell;
class IShipyard;

VCMI_LIB_NAMESPACE_END

class CCallback;
class CAdvMapInt;
class CHeroWindow;
enum class EMapAnimRedrawStatus;
class CFadeAnimation;

struct MapDrawingInfo;

/// Holds information about which tiles of the terrain are shown/not shown at the screen
class CTerrainRect : public CIntObject
{
	SDL_Surface * fadeSurface;
	EMapAnimRedrawStatus lastRedrawStatus;
	std::shared_ptr<CFadeAnimation> fadeAnim;

	int3 swipeInitialMapPos;
	int3 swipeInitialRealPos;
	bool isSwiping;
	static constexpr float SwipeTouchSlop = 16.0f;

	void handleHover(const SDL_MouseMotionEvent & sEvent);
	void handleSwipeMove(const SDL_MouseMotionEvent & sEvent);
	/// handles start/finish of swipe (press/release of corresponding button); returns true if state change was handled
	bool handleSwipeStateChange(bool btnPressed);
public:
	int tilesw, tilesh; //width and height of terrain to blit in tiles
	int3 curHoveredTile;
	int moveX, moveY; //shift between actual position of screen and the one we wil blit; ranges from -31 to 31 (in pixels)
	CGPath * currentPath;

	CTerrainRect();
	virtual ~CTerrainRect();
	void deactivate() override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void clickMiddle(tribool down, bool previousState) override;
	void hover(bool on) override;
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override;
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	void showAnim(SDL_Surface * to);
	void showPath(const Rect &extRect, SDL_Surface * to);
	int3 whichTileIsIt(const int x, const int y); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos
	/// @returns number of visible tiles on screen respecting current map scaling
	int3 tileCountOnScreen();
	/// animates view by caching current surface and crossfading it with normal screen
	void fadeFromCurrentView();
	bool needsAnimUpdate();
};

