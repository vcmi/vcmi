/*
 * CTerrainRect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../../lib/int3.h"

VCMI_LIB_NAMESPACE_BEGIN
struct CGPath;
VCMI_LIB_NAMESPACE_END

enum class EMapAnimRedrawStatus;
class CFadeAnimation;
class MapView;

/// Holds information about which tiles of the terrain are shown/not shown at the screen
class CTerrainRect : public CIntObject
{
	std::shared_ptr<MapView> renderer;

	SDL_Surface * fadeSurface;
	EMapAnimRedrawStatus lastRedrawStatus;
	std::shared_ptr<CFadeAnimation> fadeAnim;

	Point swipeInitialViewPos;
	Point swipeInitialRealPos;
	bool isSwiping;

#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
	static constexpr float SwipeTouchSlop = 16.0f; // touch UI
#else
	static constexpr float SwipeTouchSlop = 1.0f; // mouse UI
#endif

	void handleHover(const Point & cursorPosition);
	void handleSwipeMove(const Point & cursorPosition);
	/// handles start/finish of swipe (press/release of corresponding button); returns true if state change was handled
	bool handleSwipeStateChange(bool btnPressed);
	int3 curHoveredTile;

	int3 whichTileIsIt(const Point & position); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos
	void showPath(const Rect &extRect, SDL_Surface * to);

	bool needsAnimUpdate();
public:
	CGPath * currentPath;

	CTerrainRect();
	~CTerrainRect();

	void moveViewBy(const Point & delta);
	void setViewCenter(const int3 & coordinates);
	void setViewCenter(const Point & position, int level);
	void setLevel(int level);

	int3  getTileCenter();
	Point getViewCenter();
	int getLevel();

	// CIntObject interface implementation
	void deactivate() override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void clickMiddle(tribool down, bool previousState) override;
	void hover(bool on) override;
	void mouseMoved (const Point & cursorPosition) override;
	//void show(SDL_Surface * to) override;
	//void showAll(SDL_Surface * to) override;

	//void showAnim(SDL_Surface * to);

	/// @returns number of visible tiles on screen respecting current map scaling
	Rect visibleTilesArea();

	/// animates view by caching current surface and crossfading it with normal screen
	void fadeFromCurrentView();
};
