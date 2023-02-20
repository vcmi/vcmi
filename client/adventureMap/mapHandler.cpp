/*
 * mapHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapHandler.h"
#include "MapRendererContext.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/TerrainHandler.h"

/*
std::shared_ptr<IImage> CMapWorldViewBlitter::objectToIcon(Obj id, si32 subId, PlayerColor owner) const
{
	int ownerIndex = 0;
	if(owner < PlayerColor::PLAYER_LIMIT)
	{
		ownerIndex = owner.getNum() * 19;
	}
	else if (owner == PlayerColor::NEUTRAL)
	{
		ownerIndex = PlayerColor::PLAYER_LIMIT.getNum() * 19;
	}

	switch(id)
	{
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::MONOLITH_TWO_WAY:
		return info->icons->getImage((int)EWorldViewIcon::TELEPORT);
	case Obj::SUBTERRANEAN_GATE:
		return info->icons->getImage((int)EWorldViewIcon::GATE);
	case Obj::ARTIFACT:
		return info->icons->getImage((int)EWorldViewIcon::ARTIFACT);
	case Obj::TOWN:
		return info->icons->getImage((int)EWorldViewIcon::TOWN + ownerIndex);
	case Obj::HERO:
		return info->icons->getImage((int)EWorldViewIcon::HERO + ownerIndex);
	case Obj::MINE:
		return info->icons->getImage((int)EWorldViewIcon::MINE_WOOD + subId + ownerIndex);
	case Obj::RESOURCE:
		return info->icons->getImage((int)EWorldViewIcon::RES_WOOD + subId + ownerIndex);
	}
	return std::shared_ptr<IImage>();
}
*/

/*
void CMapWorldViewBlitter::drawTileOverlay(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	auto drawIcon = [this,targetSurf](Obj id, si32 subId, PlayerColor owner)
	{
		auto wvIcon = this->objectToIcon(id, subId, owner);

		if(nullptr != wvIcon)
		{
			// centering icon on the object
			Point dest(realPos.x + tileSize / 2 - wvIcon->width() / 2, realPos.y + tileSize / 2 - wvIcon->height() / 2);
			wvIcon->draw(targetSurf, dest.x, dest.y);
		}
	};

	auto & objects = tile.objects;
	for(auto & object : objects)
	{
		const CGObjectInstance * obj = object.obj;
		if(!obj)
			continue;

		const bool sameLevel = obj->pos.z == pos.z;

		//FIXME: Don't read options in  a loop :v
		const bool isVisible = settings["session"]["spectate"].Bool() ? true : (*info->visibilityMap)[pos.z][pos.x][pos.y];
		const bool isVisitable = obj->visitableAt(pos.x, pos.y);

		if(sameLevel && isVisible && isVisitable)
			drawIcon(obj->ID, obj->subID, obj->tempOwner);
	}
}
*/
/*
void CMapWorldViewBlitter::drawOverlayEx(SDL_Surface * targetSurf)
{
	if(nullptr == info->additionalIcons)
		return;

	const int3 bottomRight = pos + tileCount;

	for(const ObjectPosInfo & iconInfo : *(info->additionalIcons))
	{
		if( iconInfo.pos.z != pos.z)
			continue;

		if((iconInfo.pos.x < topTile.x) || (iconInfo.pos.y < topTile.y))
			continue;

		if((iconInfo.pos.x > bottomRight.x) || (iconInfo.pos.y > bottomRight.y))
			continue;

		realPos.x = initPos.x + (iconInfo.pos.x - topTile.x) * tileSize;
		realPos.y = initPos.y + (iconInfo.pos.y - topTile.y) * tileSize;

		auto wvIcon = this->objectToIcon(iconInfo.id, iconInfo.subId, iconInfo.owner);

		if(nullptr != wvIcon)
		{
			// centering icon on the object
			Point dest(realPos.x + tileSize / 2 - wvIcon->width() / 2, realPos.y + tileSize / 2 - wvIcon->height() / 2);
			wvIcon->draw(targetSurf, dest.x, dest.y);
		}
	}
}
*/
/*
void CMapPuzzleViewBlitter::drawObjects(SDL_Surface * targetSurf, const TerrainTile2 & tile) const
{
	CMapBlitter::drawObjects(targetSurf, tile);

	// grail X mark
	if(pos.x == info->grailPos.x && pos.y == info->grailPos.y)
	{
		const auto mark = graphics->heroMoveArrows->getImage(0);
		mark->draw(targetSurf,realTileRect.x,realTileRect.y);
	}
}
*/
/*
void CMapPuzzleViewBlitter::postProcessing(SDL_Surface * targetSurf) const
{
	CSDL_Ext::applyEffect(targetSurf, info->drawBounds, static_cast<int>(!ADVOPT.puzzleSepia));
}
*/
/*
bool CMapPuzzleViewBlitter::canDrawObject(const CGObjectInstance * obj) const
{
	if (!CMapBlitter::canDrawObject(obj))
		return false;

	//don't print flaggable objects in puzzle mode
	if (obj->isVisitable())
		return false;

	if(std::find(unblittableObjects.begin(), unblittableObjects.end(), obj->ID) != unblittableObjects.end())
		return false;

	return true;
}
*/
/*
CMapPuzzleViewBlitter::CMapPuzzleViewBlitter(CMapHandler * parent)
	: CMapNormalBlitter(parent)
{
	unblittableObjects.push_back(Obj::HOLE);
}
*/

bool CMapHandler::hasOngoingAnimations()
{
	for (auto * observer : observers)
		if (observer->hasOngoingAnimations())
			return true;

	return false;
}

void CMapHandler::waitForOngoingAnimations()
{
	while (CGI->mh->hasOngoingAnimations())
	{
		auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
		boost::this_thread::sleep(boost::posix_time::milliseconds(1));
	}
}

bool CMapHandler::hasObjectHole(const int3 & pos) const
{
	//const TerrainTile2 & tt = ttiles[pos.z][pos.x][pos.y];

	//for(auto & elem : tt.objects)
	//{
	//	if(elem.obj && elem.obj->ID == Obj::HOLE)
	//		return true;
	//}
	return false;
}

void CMapHandler::getTerrainDescr(const int3 & pos, std::string & out, bool isRMB) const
{
	const TerrainTile & t = map->getTile(pos);

	if(t.hasFavorableWinds())
	{
		out = CGI->objtypeh->getObjectName(Obj::FAVORABLE_WINDS, 0);
		return;
	}
	//const TerrainTile2 & tt = ttiles[pos.z][pos.x][pos.y];
	bool isTile2Terrain = false;
	out.clear();

	//for(auto & elem : tt.objects)
	//{
	//	if(elem.obj)
	//	{
	//		out = elem.obj->getObjectName();
	//		if(elem.obj->ID == Obj::HOLE)
	//			return;

	//		isTile2Terrain = elem.obj->isTile2Terrain();
	//		break;
	//	}
	//}

	if(!isTile2Terrain || out.empty())
		out = t.terType->getNameTranslated();

	if(t.getDiggingStatus(false) == EDiggingStatus::CAN_DIG)
	{
		out = boost::str(boost::format(isRMB ? "%s\r\n%s" : "%s %s") // New line for the Message Box, space for the Status Bar
			% out 
			% CGI->generaltexth->allTexts[330]); // 'digging ok'
	}
}

bool CMapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
{
	if (!a)
		return true;
	if (!b)
		return false;
	if (a->appearance->printPriority != b->appearance->printPriority)
		return a->appearance->printPriority > b->appearance->printPriority;

	if(a->pos.y != b->pos.y)
		return a->pos.y < b->pos.y;

	if(b->ID==Obj::HERO && a->ID!=Obj::HERO)
		return true;
	if(b->ID!=Obj::HERO && a->ID==Obj::HERO)
		return false;

	if(!a->isVisitable() && b->isVisitable())
		return true;
	if(!b->isVisitable() && a->isVisitable())
		return false;
	if(a->pos.x < b->pos.x)
		return true;
	return false;
}

CMapHandler::CMapHandler(const CMap * map)
	: map(map)
{
}

const CMap * CMapHandler::getMap()
{
	return map;
}

bool CMapHandler::isInMap( const int3 & tile)
{
	return map->isInTheMap(tile);
}

std::vector<std::string> CMapHandler::getAmbientSounds(const int3 & tile)
{
	std::vector<std::string> result;

	//for(auto & ttObj : ttiles[tile.z][tile.x][tile.y].objects)
	//{
	//	if(ttObj.ambientSound)
	//		result.push_back(ttObj.ambientSound.get());
	//}
	if(map->isCoastalTile(tile))
		result.emplace_back("LOOPOCEA");

	return result;
}

void CMapHandler::onObjectFadeIn(const CGObjectInstance * obj)
{
	for (auto * observer : observers)
		observer->onObjectFadeIn(obj);
}

void CMapHandler::onObjectFadeOut(const CGObjectInstance * obj)
{
	for (auto * observer : observers)
		observer->onObjectFadeOut(obj);
}

void CMapHandler::onObjectInstantAdd(const CGObjectInstance * obj)
{
	for (auto * observer : observers)
		observer->onObjectInstantAdd(obj);
}

void CMapHandler::onObjectInstantRemove(const CGObjectInstance * obj)
{
	for (auto * observer : observers)
		observer->onObjectInstantRemove(obj);
}

void CMapHandler::onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	for (auto * observer : observers)
		observer->onHeroTeleported(obj, from, dest);
}

void CMapHandler::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	for (auto * observer : observers)
		observer->onHeroMoved(obj, from, dest);
}

void CMapHandler::onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == from);
	for (auto * observer : observers)
		observer->onHeroRotated(obj, from, dest);
}

void CMapHandler::addMapObserver(IMapObjectObserver * object)
{
	observers.push_back(object);
}

void CMapHandler::removeMapObserver(IMapObjectObserver * object)
{
	vstd::erase(observers, object);
}

IMapObjectObserver::IMapObjectObserver()
{
	CGI->mh->addMapObserver(this);
}

IMapObjectObserver::~IMapObjectObserver()
{
	CGI->mh->removeMapObserver(this);
}
