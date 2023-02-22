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
struct ObjectPosInfo;
VCMI_LIB_NAMESPACE_END

class MapView;

/// Holds information about which tiles of the terrain are shown/not shown at the screen
class CTerrainRect : public CIntObject
{
	std::shared_ptr<MapView> renderer;

	bool swipeEnabled;
	bool swipeMovementRequested;
	Point swipeTargetPosition;
	Point swipeInitialViewPos;
	Point swipeInitialRealPos;
	bool isSwiping;

	void handleHover(const Point & cursorPosition);
	void handleSwipeMove(const Point & cursorPosition);
	/// handles start/finish of swipe (press/release of corresponding button); returns true if state change was handled
	bool handleSwipeStateChange(bool btnPressed);
	int3 curHoveredTile;

	int3 whichTileIsIt(const Point & position); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos

	Point getViewCenter();

public:
	CTerrainRect();

	/// Handle swipe & selection of object
	void setViewCenter(const int3 & coordinates);
	void setViewCenter(const Point & position, int level);

	/// Edge scrolling
	void moveViewBy(const Point & delta);

	/// Toggle undeground view button
	void setLevel(int level);
	int getLevel();

	/// World view & View Earth/Air spells
	void setTerrainVisibility(bool showAllTerrain);
	void setOverlayVisibility(const std::vector<ObjectPosInfo> & objectPositions);
	void setTileSize(int sizePixels);

	/// Minimap access
	/// @returns number of visible tiles on screen respecting current map scaling
	Rect visibleTilesArea();

	// CIntObject interface implementation
	void deactivate() override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void clickMiddle(tribool down, bool previousState) override;
	void hover(bool on) override;
	void mouseMoved (const Point & cursorPosition) override;
	void show(SDL_Surface * to) override;
	//void showAll(SDL_Surface * to) override;

	//void showAnim(SDL_Surface * to);
};
