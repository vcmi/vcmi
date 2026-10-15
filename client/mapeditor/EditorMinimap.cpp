/*
 * EditorMinimap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorMinimap.h"

#include "EditorMap.h"

#include "../widgets/UiTheme.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/Graphics.h"

#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/TerrainTile.h"

namespace
{
ColorRGBA tileColor(const CMap & map, const int3 & pos)
{
	const TerrainTile & tile = map.getTile(pos);

	// if object at tile is owned - it will be colored as its owner
	for(const auto & id : tile.blockingObjects)
	{
		const auto * object = map.getObject(id);
		if(!object)
			continue;

		const PlayerColor owner = object->getOwner();
		if(owner == PlayerColor::NEUTRAL)
			return graphics->neutralColor;

		if(owner.isValidPlayer())
			return graphics->playerColors.at(owner.getNum());
	}

	if(tile.blocked() && !tile.visitable())
		return tile.getTerrain()->minimapBlocked;

	return tile.getTerrain()->minimapUnblocked;
}
}

EditorMinimap::EditorMinimap(const Rect & area, EditorMap & editor)
	: CIntObject(LCLICK | DRAG)
	, editor(editor)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;
	this->area = pos;
	mapArea = pos;
}

EditorMinimap::~EditorMinimap() = default;

void EditorMinimap::setLevel(int newLevel)
{
	level = newLevel;
	refresh();
}

void EditorMinimap::refresh()
{
	bitmap.reset();

	if(!editor.hasMap() || level >= editor.levels())
	{
		redraw();
		return;
	}

	const CMap & map = *editor.getMap();
	bitmap = std::make_unique<Canvas>(Point(map.width, map.height), CanvasScalingPolicy::IGNORE);

	for(int y = 0; y < map.height; ++y)
		for(int x = 0; x < map.width; ++x)
			bitmap->drawPoint(Point(x, y), tileColor(map, int3(x, y, level)));

	// the minimap keeps the proportions of the map
	const double scale = std::min(static_cast<double>(area.w) / map.width, static_cast<double>(area.h) / map.height);
	const Point size(static_cast<int>(map.width * scale), static_cast<int>(map.height * scale));
	mapArea = Rect(area.center() - size / 2, size);

	redraw();
}

void EditorMinimap::setViewport(const Rect & visibleTiles)
{
	viewport = visibleTiles;
	redraw();
}

void EditorMinimap::requestCenter(const Point & cursor)
{
	if(!editor.hasMap() || !onCenterRequested || mapArea.w <= 0 || mapArea.h <= 0)
		return;

	const CMap & map = *editor.getMap();
	const int x = std::clamp((cursor.x - mapArea.x) * map.width / mapArea.w, 0, map.width - 1);
	const int y = std::clamp((cursor.y - mapArea.y) * map.height / mapArea.h, 0, map.height - 1);

	onCenterRequested(Point(x, y));
}

void EditorMinimap::clickPressed(const Point & cursorPosition)
{
	requestCenter(cursorPosition);
}

void EditorMinimap::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	requestCenter(cursorPosition);
}

void EditorMinimap::showAll(Canvas & to)
{
	to.drawColor(area, Colors::BLACK);

	if(!bitmap || !editor.hasMap())
		return;

	to.drawScaled(*bitmap, mapArea.topLeft(), mapArea.dimensions());

	const CMap & map = *editor.getMap();
	const Point topLeft = mapArea.topLeft() + Point(viewport.x * mapArea.w / map.width, viewport.y * mapArea.h / map.height);
	const Point size(std::max(2, viewport.w * mapArea.w / map.width), std::max(2, viewport.h * mapArea.h / map.height));

	CanvasClipRectGuard guard(to, mapArea);
	to.drawBorder(Rect(topLeft, size), Colors::WHITE_TRUE);
}
