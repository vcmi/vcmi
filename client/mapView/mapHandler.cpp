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
#include "IMapRendererObserver.h"
#include "mapHandler.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"

bool CMapHandler::hasOngoingAnimations()
{
	for(auto * observer : observers)
		if(observer->hasOngoingAnimations())
			return true;

	return false;
}

void CMapHandler::waitForOngoingAnimations()
{
	for(auto * observer : observers)
	{
		if (observer->hasOngoingAnimations())
			observer->waitForOngoingAnimations();
	}
}

void CMapHandler::endNetwork()
{
	for(auto * observer : observers)
		observer->endNetwork();
}

std::string CMapHandler::getTerrainDescr(const int3 & pos, bool rightClick) const
{
	const TerrainTile & t = map->getTile(pos);

	// favorable winds are a tile property rather than an object, so there is no instance to ask for a name
	if(t.hasFavorableWinds())
		return GAME->translator().translate(LIBRARY->objtypeh->getObjectGroupNameTextID(Obj::FAVORABLE_WINDS));

	std::string result = t.getTerrain()->getNameTranslated();

	for(const auto & object : map->getObjects())
	{
		if(object->coveringAt(pos) && object->isTile2Terrain())
		{
			result = object->getObjectName().toString(&GAME->translator());
			break;
		}
	}

	if(GAME->interface()->cb->getTileDigStatus(pos, false) == EDiggingStatus::CAN_DIG)
	{
		// New line for the Message Box, space for the Status Bar
		MetaString description = MetaString::createFromRawString(rightClick ? "%s\r\n%s" : "%s %s");
		description.replaceRawString(result);
		description.replaceTextID("core.genrltxt.330"); // 'digging ok'
		return description.toString(&GAME->translator());
	}

	return result;
}

CMapHandler::CMapHandler(const CMap * map)
	: map(map)
{
}

const CMap * CMapHandler::getMap()
{
	return map;
}

bool CMapHandler::isInMap(const int3 & tile)
{
	return map->isInTheMap(tile);
}

void CMapHandler::onObjectFadeIn(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	for(auto * observer : observers)
		observer->onObjectFadeIn(obj, initiator);
}

void CMapHandler::onObjectFadeOut(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	for(auto * observer : observers)
		observer->onObjectFadeOut(obj, initiator);
}

void CMapHandler::onBeforeHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	for(auto * observer : observers)
		observer->onBeforeHeroEmbark(obj, from, dest);
}

void CMapHandler::onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	for(auto * observer : observers)
		observer->onAfterHeroEmbark(obj, from, dest);
}

void CMapHandler::onBeforeHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	for(auto * observer : observers)
		observer->onBeforeHeroDisembark(obj, from, dest);
}

void CMapHandler::onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	for(auto * observer : observers)
		observer->onAfterHeroDisembark(obj, from, dest);
}

void CMapHandler::onObjectInstantAdd(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	for(auto * observer : observers)
		observer->onObjectInstantAdd(obj, initiator);
}

void CMapHandler::onObjectInstantRemove(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	for(auto * observer : observers)
		observer->onObjectInstantRemove(obj, initiator);
}

void CMapHandler::onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	for(auto * observer : observers)
		observer->onAfterHeroTeleported(obj, from, dest);
}

void CMapHandler::onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == from);
	for(auto * observer : observers)
		observer->onBeforeHeroTeleported(obj, from, dest);
}

void CMapHandler::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(obj->pos == dest);
	for(auto * observer : observers)
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
	GAME->map().addMapObserver(this);
}

IMapObjectObserver::~IMapObjectObserver()
{
	GAME->map().removeMapObserver(this);
}
