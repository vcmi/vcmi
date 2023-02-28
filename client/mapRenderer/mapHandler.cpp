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
#include "IMapRendererObserver.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CCallback.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/TerrainHandler.h"

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

std::string CMapHandler::getTerrainDescr(const int3 & pos, bool rightClick) const
{
	const TerrainTile & t = map->getTile(pos);

	if(t.hasFavorableWinds())
		return CGI->objtypeh->getObjectName(Obj::FAVORABLE_WINDS, 0);

	std::string result = t.terType->getNameTranslated();

	for(const auto & object : map->objects)
	{
		if(object->coveringAt(pos.x, pos.y) && object->pos.z == pos.z && object->isTile2Terrain())
		{
			result = object->getObjectName();
			break;
		}
	}

	if(LOCPLINT->cb->getTileDigStatus(pos, false) == EDiggingStatus::CAN_DIG)
	{
		return boost::str(boost::format(rightClick ? "%s\r\n%s" : "%s %s") // New line for the Message Box, space for the Status Bar
			% result
			% CGI->generaltexth->allTexts[330]); // 'digging ok'
	}

	return result;
}

bool CMapHandler::compareObjectBlitOrder(const CGObjectInstance * a, const CGObjectInstance * b)
{
	if (!a)
		return true;
	if (!b)
		return false;

	if (a->appearance->printPriority != 0 || b->appearance->printPriority != 0)
	{
		if (a->appearance->printPriority != b->appearance->printPriority)
			return a->appearance->printPriority > b->appearance->printPriority;

		//H3 behavior: order of two background objects depends on their placement order on map
		return a->id < b->id;
	}

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

	//H3 behavior: order of two background objects depends on their placement order on map
	return a->id < b->id;

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

void CMapHandler::onBeforeHeroEmbark(const CGHeroInstance *obj, const int3 &from, const int3 &dest)
{
	for (auto * observer : observers)
		observer->onBeforeHeroEmbark(obj, from, dest);
}

void CMapHandler::onAfterHeroEmbark(const CGHeroInstance *obj, const int3 &from, const int3 &dest)
{
	for (auto * observer : observers)
		observer->onAfterHeroEmbark(obj, from, dest);
}

void CMapHandler::onBeforeHeroDisembark(const CGHeroInstance *obj, const int3 &from, const int3 &dest)
{
	for (auto * observer : observers)
		observer->onBeforeHeroDisembark(obj, from, dest);
}

void CMapHandler::onAfterHeroDisembark(const CGHeroInstance *obj, const int3 &from, const int3 &dest)
{
	for (auto * observer : observers)
		observer->onAfterHeroDisembark(obj, from, dest);
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

void CMapHandler::onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	for (auto * observer : observers)
		observer->onAfterHeroTeleported(obj, from, dest);
}

void CMapHandler::onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == from);
	for (auto * observer : observers)
		observer->onBeforeHeroTeleported(obj, from, dest);
}

void CMapHandler::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	for (auto * observer : observers)
		observer->onHeroMoved(obj, from, dest);
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
