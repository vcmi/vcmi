/*
 * CMinimap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN
class ColorRGBA;
VCMI_LIB_NAMESPACE_END

class Canvas;
class CMinimap;

class CMinimapInstance : public CIntObject
{
	CMinimap * parent;
	std::unique_ptr<Canvas> minimap;
	int level;

	//get color of selected tile on minimap
	ColorRGBA getTileColor(const int3 & pos) const;

	void redrawMinimap();
public:
	CMinimapInstance(CMinimap * parent, int level);

	void showAll(Canvas & to) override;
	void refreshTile(const int3 & pos);
};

/// Minimap which is displayed at the right upper corner of adventure map
class CMinimap : public CIntObject
{
	std::shared_ptr<CPicture> aiShield; //the graphic displayed during AI turn
	std::shared_ptr<CMinimapInstance> minimap;
	Rect screenArea;
	int level;

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void hover (bool on) override;
	void mouseMoved (const Point & cursorPosition) override;

	/// relocates center of adventure map screen to currently hovered tile
	void moveAdvMapSelection();

protected:
	/// computes coordinates of tile below cursor pos
	int3 pixelToTile(const Point & cursorPos) const;

	/// computes position of tile within minimap instance
	Point tileToPixels(const int3 & position) const;

public:
	explicit CMinimap(const Rect & position);

	void onMapViewMoved(const Rect & visibleArea, int mapLevel);
	void update();
	void setAIRadar(bool on);

	void showAll(Canvas & to) override;

	void updateTiles(std::unordered_set<int3> positions);
};

