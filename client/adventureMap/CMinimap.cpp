/*
 * CMinimap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMinimap.h"

#include "AdventureMapInterface.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/MouseButton.h"
#include "../gui/WindowHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/Graphics.h"
#include "../render/IRenderHandler.h"
#include "../widgets/Images.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/texts/CGeneralTextHandler.h"

ColorRGBA CMinimapInstance::getTileColor(const int3 & pos) const
{
	const TerrainTile * tile = GAME->interface()->cb->getTile(pos, false);

	// if tile is not visible it will be black on minimap
	if(!tile)
		return Colors::BLACK;

	// if object at tile is owned - it will be colored as its owner
	for (const ObjectInstanceID objectID : tile->blockingObjects)
	{
		const auto * obj = GAME->interface()->cb->getObj(objectID);
		PlayerColor player = obj->getOwner();
		if(player == PlayerColor::NEUTRAL)
			return graphics->neutralColor;

		if (settings["adventure"]["minimapShowHeroes"].Bool() && obj->ID == MapObjectID::HERO)
			continue;

		if (player.isValidPlayer())
			return graphics->playerColors[player.getNum()];
	}

	if (tile->blocked() && !tile->visitable())
		return tile->getTerrain()->minimapBlocked;
	else
		return tile->getTerrain()->minimapUnblocked;
}

void CMinimapInstance::refreshTile(const int3 &tile)
{
	if (level == tile.z)
		minimap->drawPoint(Point(tile.x, tile.y), getTileColor(tile));
}

void CMinimapInstance::redrawMinimap()
{
	int3 mapSizes = GAME->interface()->cb->getMapSize();

	for (int y = 0; y < mapSizes.y; ++y)
		for (int x = 0; x < mapSizes.x; ++x)
			minimap->drawPoint(Point(x, y), getTileColor(int3(x, y, level)));
}

CMinimapInstance::CMinimapInstance(const Point & position, const Point & dimensions, int Level):
	minimap(new Canvas(Point(GAME->interface()->cb->getMapSize().x, GAME->interface()->cb->getMapSize().y), CanvasScalingPolicy::IGNORE)),
	level(Level)
{
	pos += position;
	pos.w = dimensions.x;
	pos.h = dimensions.y;
	redrawMinimap();
}

CMinimapInstance::~CMinimapInstance() = default;

void CMinimapInstance::showAll(Canvas & to)
{
	to.drawScaled(*minimap, pos.topLeft(), pos.dimensions());
}

CMinimap::CMinimap(const Rect & position)
	: CIntObject(LCLICK | SHOW_POPUP | DRAG | MOVE | GESTURE, position.topLeft())
	, heroIcon(ENGINE->renderHandler().loadImage(ImagePath::builtin("minimapIcons/hero"), EImageBlitMode::WITH_SHADOW_AND_FLAG_COLOR))
	, level(0)
{
	OBJECT_CONSTRUCTION;

	double maxSideLengthSrc = std::max(GAME->interface()->cb->getMapSize().x, GAME->interface()->cb->getMapSize().y);
	double maxSideLengthDst = std::max(position.w, position.h);
	double resize = maxSideLengthSrc / maxSideLengthDst;
	Point newMinimapSize(GAME->interface()->cb->getMapSize().x/ resize, GAME->interface()->cb->getMapSize().y / resize);
	Point offset = Point((std::max(newMinimapSize.x, newMinimapSize.y) - newMinimapSize.x) / 2, (std::max(newMinimapSize.x, newMinimapSize.y) - newMinimapSize.y) / 2);

	pos.x += offset.x;
	pos.y += offset.y;
	pos.w = newMinimapSize.x;
	pos.h = newMinimapSize.y;

	aiShield = std::make_shared<CPicture>(ImagePath::builtin("AIShield"), -offset);
	aiShield->disable();
}

int3 CMinimap::pixelToTile(const Point & cursorPos) const
{
	// 0 = top-left corner, 1 = bottom-right corner
	double dx = static_cast<double>(cursorPos.x) / pos.w;
	double dy = static_cast<double>(cursorPos.y) / pos.h;

	int3 mapSizes = GAME->interface()->cb->getMapSize();

	int tileX(std::round(mapSizes.x * dx));
	int tileY(std::round(mapSizes.y * dy));

	return int3(tileX, tileY, level);
}

Point CMinimap::tileToPixels(const int3 &tile) const
{
	int3 mapSizes = GAME->interface()->cb->getMapSize();

	double stepX = static_cast<double>(pos.w) / mapSizes.x;
	double stepY = static_cast<double>(pos.h) / mapSizes.y;

	int x = static_cast<int>(stepX * (tile.x + 0.5));
	int y = static_cast<int>(stepY * (tile.y + 0.5));

	return Point(x,y);
}

void CMinimap::moveAdvMapSelection(const Point & positionGlobal)
{
	int3 newLocation = pixelToTile(positionGlobal - pos.topLeft());
	adventureInt->centerOnTile(newLocation);

	if (!(adventureInt->isActive()))
		ENGINE->windows().totalRedraw(); //redraw this as well as inactive adventure map
	else
		redraw();//redraw only this
}

void CMinimap::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	if (pos.isInside(currentPosition))
		moveAdvMapSelection(currentPosition);
}

void CMinimap::clickPressed(const Point & cursorPosition)
{
	moveAdvMapSelection(cursorPosition);
}

void CMinimap::showPopupWindow(const Point & cursorPosition)
{
	CRClickPopup::createAndPush(LIBRARY->generaltexth->zelp[291].second);
}

void CMinimap::hover(bool on)
{
	if(on)
		ENGINE->statusbar()->write(LIBRARY->generaltexth->zelp[291].first);
	else
		ENGINE->statusbar()->clear();
}

void CMinimap::mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	moveAdvMapSelection(cursorPosition);
}

void CMinimap::showAll(Canvas & to)
{
	CanvasClipRectGuard guard(to, aiShield->pos);
	CIntObject::showAll(to);

	if(minimap)
	{
		int3 mapSizes = GAME->interface()->cb->getMapSize();

		Canvas clippedTarget(to, pos);

		if (settings["adventure"]["minimapShowHeroes"].Bool())
		{
			for (const auto objectID : visibleHeroes)
			{
				const auto * object = GAME->interface()->cb->getObj(objectID);

				if (object->anchorPos().z != level)
					continue;

				heroIcon->setOverlayColor(graphics->playerColors[object->getOwner().getNum()]);
				clippedTarget.draw(heroIcon, tileToPixels(object->visitablePos()) - heroIcon->dimensions() / 2);
			}
		}

		//draw radar
		Rect radar =
			{
				screenArea.x * pos.w / mapSizes.x,
				screenArea.y * pos.h / mapSizes.y,
				screenArea.w * pos.w / mapSizes.x - 1,
				screenArea.h * pos.h / mapSizes.y - 1
			};
		clippedTarget.drawBorderDashed(radar, Colors::PURPLE);
	}
}

void CMinimap::update()
{
	if(aiShield->recActions & UPDATE) //AI turn is going on. There is no need to update minimap
		return;

	OBJECT_CONSTRUCTION;
	minimap = std::make_shared<CMinimapInstance>(Point(0,0), pos.dimensions(), level);
	updateVisibleHeroes();
	redraw();
}

void CMinimap::onMapViewMoved(const Rect & visibleArea, int mapLevel)
{
	if (screenArea == visibleArea && level == mapLevel)
		return;

	screenArea = visibleArea;

	if(level != mapLevel)
	{
		level = mapLevel;
		update();
	}
	else
	{
		setRedrawParent(true); // needed for non square map to redraw black background when viewarea rectangle is moved
		redraw();
	}
}

void CMinimap::setAIRadar(bool on)
{
	if(on)
	{
		aiShield->enable();
		minimap.reset();
	}
	else
	{
		aiShield->disable();
		update();
	}
	redraw();
}

void CMinimap::updateVisibleHeroes()
{
	visibleHeroes.clear();

	for (const auto & player : PlayerColor::ALL_PLAYERS())
	{
		if (GAME->interface()->cb->getPlayerStatus(player, false) != EPlayerStatus::INGAME)
			continue;

		for (const auto & hero : GAME->interface()->cb->getHeroes(player))
			visibleHeroes.push_back(hero->id);
	}
}

void CMinimap::updateTiles(const FowTilesType & positions)
{
	if(minimap)
	{
		for (auto const & tile : positions)
			minimap->refreshTile(tile);
	}

	updateVisibleHeroes();
	redraw();
}
