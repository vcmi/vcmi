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
	: fadeSurface(nullptr),
	  lastRedrawStatus(EMapAnimRedrawStatus::OK),
	  fadeAnim(std::make_shared<CFadeAnimation>()),
	  curHoveredTile(-1,-1,-1),
	  currentPath(nullptr)
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
	{
#endif
		if(down == false)
			return;
#if defined(VCMI_MOBILE)
	}
#endif
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
		if(abs(cursorPosition.x - swipeInitialRealPos.x) > SwipeTouchSlop ||
		   abs(cursorPosition.y - swipeInitialRealPos.y) > SwipeTouchSlop)
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
	else if(isSwiping) // only accept this touch if it wasn't a swipe
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
void CTerrainRect::showPath(const Rect & extRect, SDL_Surface * to)
{
/*
	const static int pns[9][9] = {
				{16, 17, 18,  7, -1, 19,  6,  5, -1},
				{ 8,  9, 18,  7, -1, 19,  6, -1, 20},
				{ 8,  1, 10,  7, -1, 19, -1, 21, 20},
				{24, 17, 18, 15, -1, -1,  6,  5,  4},
				{-1, -1, -1, -1, -1, -1, -1, -1, -1},
				{ 8,  1,  2, -1, -1, 11, 22, 21, 20},
				{24, 17, -1, 23, -1,  3, 14,  5,  4},
				{24, -1,  2, 23, -1,  3, 22, 13,  4},
				{-1,  1,  2, 23, -1,  3, 22, 21, 12}
			}; //table of magic values TODO meaning, change variable name

	for (int i = 0; i < -1 + (int)currentPath->nodes.size(); ++i)
	{
		const int3 &curPos = currentPath->nodes[i].coord, &nextPos = currentPath->nodes[i+1].coord;
		if(curPos.z != adventureInt->position.z)
			continue;

		int pn=-1;//number of picture
		if (i==0) //last tile
		{
			int x = 32*(curPos.x-adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(curPos.y-adventureInt->position.y)+CGI->mh->offsetY + pos.y;
			if (x<0 || y<0 || x>pos.w || y>pos.h)
				continue;
			pn=0;
		}
		else
		{
			const int3 &prevPos = currentPath->nodes[i-1].coord;
			std::vector<CGPathNode> & cv = currentPath->nodes;

			// Vector directions
			//  0   1   2
			//    \ | /
			//  3 - 4 - 5
			//    / | \
			//  6   7  8
			//For example:
			//  |
			//  |__\
			//     /
			// is id1=7, id2=5 (pns[7][5])
			//
			bool pathContinuous = curPos.areNeighbours(nextPos) && curPos.areNeighbours(prevPos);
			if(pathContinuous && cv[i].action != CGPathNode::EMBARK && cv[i].action != CGPathNode::DISEMBARK)
			{
				int id1=(curPos.x-nextPos.x+1)+3*(curPos.y-nextPos.y+1);   //Direction of entering vector
				int id2=(cv[i-1].coord.x-curPos.x+1)+3*(cv[i-1].coord.y-curPos.y+1); //Direction of exiting vector
				pn=pns[id1][id2];
			}
			else //path discontinuity or sea/land transition (eg. when moving through Subterranean Gate or Boat)
			{
				pn = 0;
			}
		}
		if (currentPath->nodes[i].turns)
			pn+=25;
		if (pn>=0)
		{
			const auto arrow = graphics->heroMoveArrows->getImage(pn);

			int x = 32*(curPos.x-adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(curPos.y-adventureInt->position.y)+CGI->mh->offsetY + pos.y;
			if (x< -32 || y< -32 || x>pos.w || y>pos.h)
				continue;
			int hvx = (x + arrow->width())  - (pos.x + pos.w),
				hvy = (y + arrow->height()) - (pos.y + pos.h);

			Rect prevClip;
			CSDL_Ext::getClipRect(to, prevClip);
			CSDL_Ext::setClipRect(to, extRect); //preventing blitting outside of that rect

			if(ADVOPT.smoothMove) //version for smooth hero move, with pos shifts
			{
				if (hvx<0 && hvy<0)
				{
					arrow->draw(to, x + moveX, y + moveY);
				}
				else if(hvx<0)
				{
					Rect srcRect (Point(0, 0), Point(arrow->height() - hvy, arrow->width()));
					arrow->draw(to, x + moveX, y + moveY, &srcRect);
				}
				else if (hvy<0)
				{
					Rect srcRect (Point(0, 0), Point(arrow->height(), arrow->width() - hvx));
					arrow->draw(to, x + moveX, y + moveY, &srcRect);
				}
				else
				{
					Rect srcRect (Point(0, 0), Point(arrow->height() - hvy, arrow->width() - hvx));
					arrow->draw(to, x + moveX, y + moveY, &srcRect);
				}
			}
			else //standard version
			{
				if (hvx<0 && hvy<0)
				{
					arrow->draw(to, x, y);
				}
				else if(hvx<0)
				{
					Rect srcRect (Point(0, 0), Point(arrow->height() - hvy, arrow->width()));
					arrow->draw(to, x, y, &srcRect);
				}
				else if (hvy<0)
				{
					Rect srcRect (Point(0, 0), Point(arrow->height(), arrow->width() - hvx));
					arrow->draw(to, x, y, &srcRect);
				}
				else
				{
					Rect srcRect (Point(0, 0), Point(arrow->height() - hvy, arrow->width() - hvx));
					arrow->draw(to, x, y, &srcRect);
				}
			}
			CSDL_Ext::setClipRect(to, prevClip);

		}
	} //for (int i=0;i<currentPath->nodes.size()-1;i++)
*/}
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

		if (currentPath) //drawing path
		{
			showPath(pos, to);
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
	return renderer->getTileAtPoint(position - pos);
}

int3 CTerrainRect::whichTileIsIt()
{
	return whichTileIsIt(GH.getCursorPosition());
}

Rect CTerrainRect::visibleTilesArea()
{
	return renderer->getVisibleAreaTiles();
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
	renderer->setViewCenter(renderer->getViewCenter(), level);
}

void CTerrainRect::moveViewBy(const Point & delta)
{
	renderer->setViewCenter(renderer->getViewCenter() + delta, getLevel());
}

int3 CTerrainRect::getTileCenter()
{
	return renderer->getTileCenter();
}

Point CTerrainRect::getViewCenter()
{
	return renderer->getViewCenter();
}

int CTerrainRect::getLevel()
{
	return renderer->getTileCenter().z;
}
