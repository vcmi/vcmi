/*
 * EditorMinimap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class EditorMap;

/// Overview of a level of the edited map, with the part visible in the map view marked on it
class EditorMinimap : public CIntObject
{
	EditorMap & editor;
	std::unique_ptr<Canvas> bitmap;
	Rect area;
	Rect mapArea;
	Rect viewport;
	int level = 0;

	void requestCenter(const Point & cursor);

public:
	/// Called with the tile to center the map view on
	std::function<void(const Point &)> onCenterRequested;

	EditorMinimap(const Rect & area, EditorMap & editor);
	~EditorMinimap() override;

	void setLevel(int newLevel);
	/// Draws the minimap again from the map
	void refresh();
	/// Part of the map that is visible, in tiles
	void setViewport(const Rect & visibleTiles);

	void clickPressed(const Point & cursorPosition) override;
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void showAll(Canvas & to) override;
};
