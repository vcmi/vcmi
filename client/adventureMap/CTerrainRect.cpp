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

#include "mapHandler.h"
#include "MapView.h"
#include "CAdvMapInt.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../render/CAnimation.h"
#include "../render/CFadeAnimation.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../widgets/TextControls.h"
#include "../CMT.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/CPathfinder.h"

#include <SDL_surface.h>

#define ADVOPT (conf.go()->ac)

CTerrainRect::CTerrainRect()
	: fadeSurface(nullptr)
	, lastRedrawStatus(EMapAnimRedrawStatus::OK)
	, fadeAnim(std::make_shared<CFadeAnimation>())
	, curHoveredTile(-1, -1, -1)
	, isSwiping(false)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.x=ADVOPT.advmapX;
	pos.y=ADVOPT.advmapY;
	pos.w=ADVOPT.advmapW;
	pos.h=ADVOPT.advmapH;
	addUsedEvents(LCLICK | RCLICK | MCLICK | HOVER | MOVE);

	renderer = std::make_shared<MapView>( Point(0,0), pos.dimensions() );
}

CTerrainRect::~CTerrainRect()
{
	if(fadeSurface)
		SDL_FreeSurface(fadeSurface);
}

void CTerrainRect::setViewCenter(const int3 &coordinates)
{
	renderer->setViewCenter(coordinates);
}

void CTerrainRect::setViewCenter(const Point & position, int level)
{
	renderer->setViewCenter(position, level);
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

#if defined(VCMI_MOBILE)
	if(adventureInt->swipeEnabled)
	{
		if(handleSwipeStateChange((bool)down == true))
		{
			return; // if swipe is enabled, we don't process "down" events and wait for "up" (to make sure this wasn't a swiping gesture)
		}
	}
	else
#endif
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
#if defined(VCMI_MOBILE)
	if(adventureInt->swipeEnabled && isSwiping)
		return;
#endif
	if(adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		return;
	int3 mp = whichTileIsIt();

	if(CGI->mh->map->isInTheMap(mp) && down)
		adventureInt->tileRClicked(mp);
}

void CTerrainRect::clickMiddle(tribool down, bool previousState)
{
	handleSwipeStateChange((bool)down == true);
}

void CTerrainRect::mouseMoved(const Point & cursorPosition)
{
	handleHover(cursorPosition);

	if(!adventureInt->swipeEnabled)
		return;

	handleSwipeMove(cursorPosition);
}

void CTerrainRect::handleSwipeMove(const Point & cursorPosition)
{
#if defined(VCMI_MOBILE)
	if(!GH.isMouseButtonPressed() || GH.multifinger) // any "button" is enough on mobile
		return;
#else
	if(!GH.isMouseButtonPressed(MouseButton::MIDDLE)) // swipe only works with middle mouse on other platforms
		return;
#endif

	if(!isSwiping)
	{
		// try to distinguish if this touch was meant to be a swipe or just fat-fingering press
		if(std::abs(cursorPosition.x - swipeInitialRealPos.x) > SwipeTouchSlop ||
		   std::abs(cursorPosition.y - swipeInitialRealPos.y) > SwipeTouchSlop)
		{
			isSwiping = true;
		}
	}

	if(isSwiping)
	{
		adventureInt->swipeTargetPosition.x = swipeInitialViewPos.x + swipeInitialRealPos.x - cursorPosition.x;
		adventureInt->swipeTargetPosition.y = swipeInitialViewPos.y + swipeInitialRealPos.y - cursorPosition.y;
		adventureInt->swipeMovementRequested = true;
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
	int3 pom = adventureInt->verifyPos(tHovered);

	if(tHovered != pom) //tile outside the map
	{
		CCS->curh->set(Cursor::Map::POINTER);
		return;
	}

	if (pom != curHoveredTile)
	{
		curHoveredTile = pom;
		adventureInt->tileHovered(pom);
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

/*
void CTerrainRect::show(SDL_Surface * to)
{
	if (adventureInt->mode == EAdvMapMode::NORMAL)
	{
		MapDrawingInfo info(adventureInt->position, LOCPLINT->cb->getVisibilityMap(), pos);
		info.otherheroAnim = true;
		info.anim = adventureInt->anim;
		info.heroAnim = adventureInt->heroAnim;
		if (ADVOPT.smoothMove)
			info.movement = int3(moveX, moveY, 0);

		lastRedrawStatus = CGI->mh->drawTerrainRectNew(to, &info);
		if (fadeAnim->isFading())
		{
			Rect r(pos);
			fadeAnim->update();
			fadeAnim->draw(to, r.topLeft());
		}
	}
}

void CTerrainRect::showAll(SDL_Surface * to)
{
	// world view map is static and doesn't need redraw every frame
	if (adventureInt->mode == EAdvMapMode::WORLD_VIEW)
	{
		MapDrawingInfo info(adventureInt->position, LOCPLINT->cb->getVisibilityMap(), pos, adventureInt->worldViewIcons);
		info.scaled = true;
		info.scale = adventureInt->worldViewScale;
		adventureInt->worldViewOptions.adjustDrawingInfo(info);
		CGI->mh->drawTerrainRectNew(to, &info);
	}
}

void CTerrainRect::showAnim(SDL_Surface * to)
{
	if (!needsAnimUpdate())
		return;

	if (fadeAnim->isFading() || lastRedrawStatus == EMapAnimRedrawStatus::REDRAW_REQUESTED)
		show(to);
}
*/
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

void CTerrainRect::fadeFromCurrentView()
{
	if (!ADVOPT.screenFading)
		return;
	if (adventureInt->mode == EAdvMapMode::WORLD_VIEW)
		return;

	if (!fadeSurface)
		fadeSurface = CSDL_Ext::newSurface(pos.w, pos.h);
	CSDL_Ext::blitSurface(screen, fadeSurface, Point(0,0));
	fadeAnim->init(CFadeAnimation::EMode::OUT, fadeSurface);
}

bool CTerrainRect::needsAnimUpdate()
{
	return fadeAnim->isFading() || lastRedrawStatus == EMapAnimRedrawStatus::REDRAW_REQUESTED;
}

void CTerrainRect::setLevel(int level)
{
	renderer->setViewCenter(renderer->getModel()->getMapViewCenter(), level);
}

void CTerrainRect::moveViewBy(const Point & delta)
{
	renderer->setViewCenter(renderer->getModel()->getMapViewCenter() + delta, getLevel());
}

int3 CTerrainRect::getTileCenter()
{
	return renderer->getModel()->getTileCenter();
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
	renderer->setTileSize(Point(sizePixels, sizePixels));
}
