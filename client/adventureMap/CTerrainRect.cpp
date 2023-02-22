/*
 * CTerrainRect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTerrainRect.h"

#include "CAdvMapInt.h"

#include "../CGameInfo.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../mapRenderer/mapHandler.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../widgets/TextControls.h"
#include "../mapRenderer/MapView.h"
#include "../mapRenderer/MapViewController.h"
#include "../mapRenderer/MapViewModel.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/CPathfinder.h"

#define ADVOPT (conf.go()->ac)

CTerrainRect::CTerrainRect()
	: curHoveredTile(-1, -1, -1)
	, isSwiping(false)
#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
	, swipeEnabled(settings["general"]["swipe"].Bool())
#else
	, swipeEnabled(settings["general"]["swipeDesktop"].Bool())
#endif
	, swipeMovementRequested(false)
	, swipeTargetPosition(Point(0, 0))
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.x=ADVOPT.advmapX;
	pos.y=ADVOPT.advmapY;
	pos.w=ADVOPT.advmapW;
	pos.h=ADVOPT.advmapH;
	addUsedEvents(LCLICK | RCLICK | MCLICK | HOVER | MOVE);

	renderer = std::make_shared<MapView>( Point(0,0), pos.dimensions() );
}

void CTerrainRect::setViewCenter(const int3 &coordinates)
{
	renderer->getController()->setViewCenter(coordinates);
}

void CTerrainRect::setViewCenter(const Point & position, int level)
{
	renderer->getController()->setViewCenter(position, level);
}

void CTerrainRect::deactivate()
{
	CIntObject::deactivate();
	curHoveredTile = int3(-1,-1,-1); //we lost info about hovered tile when disabling
}

void CTerrainRect::clickLeft(tribool down, bool previousState)
{
	if(adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		return;
	if(indeterminate(down))
		return;

	if(swipeEnabled)
	{
		if(handleSwipeStateChange((bool)down == true))
		{
			return; // if swipe is enabled, we don't process "down" events and wait for "up" (to make sure this wasn't a swiping gesture)
		}
	}
	else
	{
		if(down == false)
			return;
	}

	int3 mp = whichTileIsIt();
	if(mp.x < 0 || mp.y < 0 || mp.x >= LOCPLINT->cb->getMapSize().x || mp.y >= LOCPLINT->cb->getMapSize().y)
		return;

	adventureInt->tileLClicked(mp);
}

void CTerrainRect::clickRight(tribool down, bool previousState)
{
	if(isSwiping)
		return;

	if(adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		return;
	int3 mp = whichTileIsIt();

	if(CGI->mh->isInMap(mp) && down)
		adventureInt->tileRClicked(mp);
}

void CTerrainRect::clickMiddle(tribool down, bool previousState)
{
	handleSwipeStateChange((bool)down == true);
}

void CTerrainRect::mouseMoved(const Point & cursorPosition)
{
	handleHover(cursorPosition);

	handleSwipeMove(cursorPosition);
}

void CTerrainRect::handleSwipeMove(const Point & cursorPosition)
{
	// unless swipe is enabled, swipe move only works with middle mouse button
	if(!swipeEnabled && !GH.isMouseButtonPressed(MouseButton::MIDDLE))
		return;

	// on mobile platforms with enabled swipe any button is enough
	if(swipeEnabled && (!GH.isMouseButtonPressed() || GH.multifinger))
		return;

	if(!isSwiping)
	{
		static constexpr int touchSwipeSlop = 16;

		// try to distinguish if this touch was meant to be a swipe or just fat-fingering press
		if(std::abs(cursorPosition.x - swipeInitialRealPos.x) > touchSwipeSlop ||
		   std::abs(cursorPosition.y - swipeInitialRealPos.y) > touchSwipeSlop)
		{
			isSwiping = true;
		}
	}

	if(isSwiping)
	{
		swipeTargetPosition.x = swipeInitialViewPos.x + swipeInitialRealPos.x - cursorPosition.x;
		swipeTargetPosition.y = swipeInitialViewPos.y + swipeInitialRealPos.y - cursorPosition.y;
		swipeMovementRequested = true;
	}
}

bool CTerrainRect::handleSwipeStateChange(bool btnPressed)
{
	if(btnPressed)
	{
		swipeInitialRealPos = Point(GH.getCursorPosition().x, GH.getCursorPosition().y);
		swipeInitialViewPos = getViewCenter();
		return true;
	}

	if(isSwiping) // only accept this touch if it wasn't a swipe
	{
		isSwiping = false;
		return true;
	}
	return false;
}

void CTerrainRect::handleHover(const Point & cursorPosition)
{
	int3 tHovered = whichTileIsIt(cursorPosition);

	if(!CGI->mh->isInMap(tHovered))
	{
		CCS->curh->set(Cursor::Map::POINTER);
		return;
	}

	if (tHovered != curHoveredTile)
	{
		curHoveredTile = tHovered;
		adventureInt->tileHovered(tHovered);
	}
}

void CTerrainRect::hover(bool on)
{
	if (!on)
	{
		GH.statusbar->clear();
		CCS->curh->set(Cursor::Map::POINTER);
	}
	//Hoverable::hover(on);
}

int3 CTerrainRect::whichTileIsIt(const Point &position)
{
	return renderer->getModel()->getTileAtPoint(position - pos);
}

int3 CTerrainRect::whichTileIsIt()
{
	return whichTileIsIt(GH.getCursorPosition());
}

Rect CTerrainRect::visibleTilesArea()
{
	return renderer->getModel()->getTilesTotalRect();
}

void CTerrainRect::setLevel(int level)
{
	renderer->getController()->setViewCenter(renderer->getModel()->getMapViewCenter(), level);
}

void CTerrainRect::moveViewBy(const Point & delta)
{
	// ignore scrolling attempts while we are swiping
	if (isSwiping || swipeMovementRequested)
		return;

	renderer->getController()->setViewCenter(renderer->getModel()->getMapViewCenter() + delta, getLevel());
}

Point CTerrainRect::getViewCenter()
{
	return renderer->getModel()->getMapViewCenter();
}

int CTerrainRect::getLevel()
{
	return renderer->getModel()->getLevel();
}

void CTerrainRect::setTileSize(int sizePixels)
{
	renderer->getController()->setTileSize(Point(sizePixels, sizePixels));
}

void CTerrainRect::setTerrainVisibility(bool showAllTerrain)
{
	renderer->getController()->setTerrainVisibility(showAllTerrain);
}

void CTerrainRect::setOverlayVisibility(const std::vector<ObjectPosInfo> & objectPositions)
{
	renderer->getController()->setOverlayVisibility(objectPositions);
}

void CTerrainRect::show(SDL_Surface * to)
{
	if(swipeMovementRequested)
	{
		setViewCenter(swipeTargetPosition, getLevel());
		CCS->curh->set(Cursor::Map::POINTER);
		swipeMovementRequested = false;
	}
	CIntObject::show(to);
}
