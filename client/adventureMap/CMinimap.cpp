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

#include "CAdvMapInt.h"

#include "../widgets/Images.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../renderSDL/SDL_PixelAccess.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMapDefines.h"

#include <SDL_surface.h>

const SDL_Color & CMinimapInstance::getTileColor(const int3 & pos)
{
	const TerrainTile * tile = LOCPLINT->cb->getTile(pos, false);

	// if tile is not visible it will be black on minimap
	if(!tile)
		return Colors::BLACK;

	// if object at tile is owned - it will be colored as its owner
	for (const CGObjectInstance *obj : tile->blockingObjects)
	{
		//heroes will be blitted later
		switch (obj->ID)
		{
			case Obj::HERO:
			case Obj::PRISON:
				continue;
		}

		PlayerColor player = obj->getOwner();
		if(player == PlayerColor::NEUTRAL)
			return *graphics->neutralColor;
		else
		if (player < PlayerColor::PLAYER_LIMIT)
			return graphics->playerColors[player.getNum()];
	}

	// else - use terrain color (blocked version or normal)
	const auto & colorPair = parent->colors.find(tile->terType->getId())->second;
	if (tile->blocked && (!tile->visitable))
		return colorPair.second;
	else
		return colorPair.first;
}
void CMinimapInstance::tileToPixels (const int3 &tile, int &x, int &y, int toX, int toY)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();

	double stepX = double(pos.w) / mapSizes.x;
	double stepY = double(pos.h) / mapSizes.y;

	x = static_cast<int>(toX + stepX * tile.x);
	y = static_cast<int>(toY + stepY * tile.y);
}

void CMinimapInstance::blitTileWithColor(const SDL_Color &color, const int3 &tile, SDL_Surface *to, int toX, int toY)
{
	//coordinates of rectangle on minimap representing this tile
	// begin - first to blit, end - first NOT to blit
	int xBegin, yBegin, xEnd, yEnd;
	tileToPixels (tile, xBegin, yBegin, toX, toY);
	tileToPixels (int3 (tile.x + 1, tile.y + 1, tile.z), xEnd, yEnd, toX, toY);

	for (int y=yBegin; y<yEnd; y++)
	{
		uint8_t *ptr = (uint8_t*)to->pixels + y * to->pitch + xBegin * minimap->format->BytesPerPixel;

		for (int x=xBegin; x<xEnd; x++)
			ColorPutter<4, 1>::PutColor(ptr, color);
	}
}

void CMinimapInstance::refreshTile(const int3 &tile)
{
	blitTileWithColor(getTileColor(int3(tile.x, tile.y, level)), tile, minimap, 0, 0);
}

void CMinimapInstance::drawScaled(int level)
{
	int3 mapSizes = LOCPLINT->cb->getMapSize();

	//size of one map tile on our minimap
	double stepX = double(pos.w) / mapSizes.x;
	double stepY = double(pos.h) / mapSizes.y;

	double currY = 0;
	for (int y=0; y<mapSizes.y; y++, currY += stepY)
	{
		double currX = 0;
		for (int x=0; x<mapSizes.x; x++, currX += stepX)
		{
			const SDL_Color &color = getTileColor(int3(x,y,level));

			//coordinates of rectangle on minimap representing this tile
			// begin - first to blit, end - first NOT to blit
			int xBegin = static_cast<int>(currX);
			int yBegin = static_cast<int>(currY);
			int xEnd = static_cast<int>(currX + stepX);
			int yEnd = static_cast<int>(currY + stepY);

			for (int y=yBegin; y<yEnd; y++)
			{
				uint8_t *ptr = (uint8_t*)minimap->pixels + y * minimap->pitch + xBegin * minimap->format->BytesPerPixel;

				for (int x=xBegin; x<xEnd; x++)
					ColorPutter<4, 1>::PutColor(ptr, color);
			}
		}
	}
}

CMinimapInstance::CMinimapInstance(CMinimap *Parent, int Level):
	parent(Parent),
	minimap(CSDL_Ext::createSurfaceWithBpp<4>(parent->pos.w, parent->pos.h)),
	level(Level)
{
	pos.w = parent->pos.w;
	pos.h = parent->pos.h;
	drawScaled(level);
}

CMinimapInstance::~CMinimapInstance()
{
	SDL_FreeSurface(minimap);
}

void CMinimapInstance::showAll(SDL_Surface * to)
{
	blitAtLoc(minimap, 0, 0, to);

	//draw heroes
	std::vector <const CGHeroInstance *> heroes = LOCPLINT->cb->getHeroesInfo(false); //TODO: do we really need separate function for drawing heroes?
	for(auto & hero : heroes)
	{
		int3 position = hero->visitablePos();
		if(position.z == level)
		{
			const SDL_Color & color = graphics->playerColors[hero->getOwner().getNum()];
			blitTileWithColor(color, position, to, pos.x, pos.y);
		}
	}
}

std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > CMinimap::loadColors()
{
	std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > ret;

	for(const auto & terrain : CGI->terrainTypeHandler->objects)
	{
		SDL_Color normal = CSDL_Ext::toSDL(terrain->minimapUnblocked);
		SDL_Color blocked = CSDL_Ext::toSDL(terrain->minimapBlocked);

		ret[terrain->getId()] = std::make_pair(normal, blocked);
	}
	return ret;
}

CMinimap::CMinimap(const Rect & position)
	: CIntObject(LCLICK | RCLICK | HOVER | MOVE, position.topLeft()),
	level(0),
	colors(loadColors())
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.w = position.w;
	pos.h = position.h;

	aiShield = std::make_shared<CPicture>("AIShield");
	aiShield->disable();
}

int3 CMinimap::translateMousePosition()
{
	// 0 = top-left corner, 1 = bottom-right corner
	double dx = double(GH.getCursorPosition().x - pos.x) / pos.w;
	double dy = double(GH.getCursorPosition().y - pos.y) / pos.h;

	int3 mapSizes = LOCPLINT->cb->getMapSize();

	int3 tile ((si32)(mapSizes.x * dx), (si32)(mapSizes.y * dy), level);
	return tile;
}

void CMinimap::moveAdvMapSelection()
{
	int3 newLocation = translateMousePosition();
	adventureInt->centerOn(newLocation);

	if (!(adventureInt->active & GENERAL))
		GH.totalRedraw(); //redraw this as well as inactive adventure map
	else
		redraw();//redraw only this
}

void CMinimap::clickLeft(tribool down, bool previousState)
{
	if(down)
		moveAdvMapSelection();
}

void CMinimap::clickRight(tribool down, bool previousState)
{
	adventureInt->handleRightClick(CGI->generaltexth->zelp[291].second, down);
}

void CMinimap::hover(bool on)
{
	if(on)
		GH.statusbar->write(CGI->generaltexth->zelp[291].first);
	else
		GH.statusbar->clear();
}

void CMinimap::mouseMoved(const Point & cursorPosition)
{
	if(mouseState(MouseButton::LEFT))
		moveAdvMapSelection();
}

void CMinimap::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if(minimap)
	{
		int3 mapSizes = LOCPLINT->cb->getMapSize();
		int3 tileCountOnScreen = adventureInt->terrain.tileCountOnScreen();

		//draw radar
		Rect oldClip;
		Rect radar =
		{
			si16(adventureInt->position.x * pos.w / mapSizes.x + pos.x),
			si16(adventureInt->position.y * pos.h / mapSizes.y + pos.y),
			ui16(tileCountOnScreen.x * pos.w / mapSizes.x),
			ui16(tileCountOnScreen.y * pos.h / mapSizes.y)
		};

		if(adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		{
			// adjusts radar so that it doesn't go out of map in world view mode (since there's no frame)
			radar.x = std::min<int>(std::max(pos.x, radar.x), pos.x + pos.w - radar.w);
			radar.y = std::min<int>(std::max(pos.y, radar.y), pos.y + pos.h - radar.h);

			if(radar.x < pos.x && radar.y < pos.y)
				return; // whole map is visible at once, no point in redrawing border
		}

		CSDL_Ext::getClipRect(to, oldClip);
		CSDL_Ext::setClipRect(to, pos);
		CSDL_Ext::drawDashedBorder(to, radar, Colors::PURPLE);
		CSDL_Ext::setClipRect(to, oldClip);
	}
}

void CMinimap::update()
{
	if(aiShield->recActions & UPDATE) //AI turn is going on. There is no need to update minimap
		return;

	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	minimap = std::make_shared<CMinimapInstance>(this, level);
	redraw();
}

void CMinimap::setLevel(int newLevel)
{
	level = newLevel;
	update();
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
	// this my happen during AI turn when this interface is inactive
	// force redraw in order to properly update interface
	GH.totalRedraw();
}

void CMinimap::hideTile(const int3 &pos)
{
	if(minimap)
		minimap->refreshTile(pos);
}

void CMinimap::showTile(const int3 &pos)
{
	if(minimap)
		minimap->refreshTile(pos);
}

