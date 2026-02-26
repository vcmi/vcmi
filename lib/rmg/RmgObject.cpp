/*
 * RmgObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RmgObject.h"
#include "RmgMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"
#include "../GameLibrary.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapObjects/CGObjectInstance.h"
#include "Functions.h"
#include "../TerrainHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

using namespace rmg;

Object::Instance::Instance(const Object& parent, std::shared_ptr<CGObjectInstance> object): dParent(parent), dObject(object)
{
	setPosition(dPosition);
}

Object::Instance::Instance(const Object& parent, std::shared_ptr<CGObjectInstance> object, const int3 & position): Instance(parent, object)
{
	setPosition(position);
}

const Area & Object::Instance::getBlockedArea() const
{
	if(dBlockedAreaCache.empty())
	{
		std::set<int3> blockedArea = dObject->getBlockedPos();
		dBlockedAreaCache.assign(rmg::Tileset(blockedArea.begin(), blockedArea.end()));
		if(dObject->isVisitable() || dBlockedAreaCache.empty())
			dBlockedAreaCache.add(dObject->visitablePos());
	}
	return dBlockedAreaCache;
}

int3 Object::Instance::getTopTile() const
{
	return object().getTopVisiblePos();
}

int3 Object::Instance::getPosition(bool isAbsolute) const
{
	if(isAbsolute)
		return dPosition + dParent.getPosition();
	else
		return dPosition;
}

int3 Object::Instance::getVisitablePosition() const
{
	return dObject->visitablePos();
}

const rmg::Area & Object::Instance::getAccessibleArea() const
{
	if(dAccessibleAreaCache.empty())
	{
		auto neighbours = rmg::Area({getVisitablePosition()}).getBorderOutside();
		// FIXME: Blocked area of removable object is also accessible area for neighbors
		rmg::Area visitable = rmg::Area(neighbours) - getBlockedArea();
		// TODO: Add in one operation to avoid multiple invalidation
		for(const auto & from : visitable.getTilesVector())
		{
			if(isVisitableFrom(from))
				dAccessibleAreaCache.add(from);
		}
	}
	return dAccessibleAreaCache;
}

void Object::Instance::setPosition(const int3 & position)
{
	dPosition = position;
	dObject->setAnchorPos(dPosition + dParent.getPosition());
	
	dBlockedAreaCache.clear();
	dAccessibleAreaCache.clear();
	dParent.clearCachedArea();
}

void Object::Instance::setPositionRaw(const int3 & position)
{
	if(!dObject->anchorPos().isValid())
	{
		dObject->setAnchorPos(dPosition + dParent.getPosition());
		dBlockedAreaCache.clear();
		dAccessibleAreaCache.clear();
		dParent.clearCachedArea();
	}
		
	auto shift = position + dParent.getPosition() - dObject->anchorPos();
	
	dAccessibleAreaCache.translate(shift);
	dBlockedAreaCache.translate(shift);
	
	dPosition = position;
	dObject->setAnchorPos(dPosition + dParent.getPosition());
}

void Object::Instance::setAnyTemplate(vstd::RNG & rng)
{
	auto templates = dObject->getObjectHandler()->getTemplates();
	if(templates.empty())
		throw rmgException(boost::str(boost::format("Did not find any graphics for object (%d,%d)") % dObject->ID % dObject->getObjTypeIndex()));

	dObject->appearance = *RandomGeneratorUtil::nextItem(templates, rng);
	dAccessibleAreaCache.clear();
	setPosition(getPosition(false));
}

void Object::Instance::setTemplate(TerrainId terrain, vstd::RNG & rng)
{
	auto templates = dObject->getObjectHandler()->getMostSpecificTemplates(terrain);

	if (templates.empty())
	{
		auto terrainName = LIBRARY->terrainTypeHandler->getById(terrain)->getNameTranslated();
		throw rmgException(boost::str(boost::format("Did not find graphics for object (%d,%d) at %s") % dObject->ID % dObject->getObjTypeIndex() % terrainName));
	}
	
	dObject->appearance = *RandomGeneratorUtil::nextItem(templates, rng);
	dAccessibleAreaCache.clear();
	setPosition(getPosition(false));
}

void Object::Instance::clear()
{
	if (onCleared)
		onCleared(*dObject);

	dBlockedAreaCache.clear();
	dAccessibleAreaCache.clear();
	dParent.clearCachedArea();
}

bool Object::Instance::isVisitableFrom(const int3 & position) const
{
	auto relPosition = position - getPosition(true);
	return dObject->appearance->isVisitableFrom(relPosition.x, relPosition.y);
}

bool Object::Instance::isBlockedVisitable() const
{
	return dObject->isBlockedVisitable();
}

bool Object::Instance::isRemovable() const
{
	return dObject->isRemovable();
}

CGObjectInstance & Object::Instance::object()
{
	return *dObject;
}

std::shared_ptr<CGObjectInstance> Object::Instance::pointer() const
{
	return dObject;
}

const CGObjectInstance & Object::Instance::object() const
{
	return *dObject;
}

Object::Object(std::shared_ptr<CGObjectInstance> object, const int3 & position):
	guarded(false),
	value(0)
{
	addInstance(object, position);
}

Object::Object(std::shared_ptr<CGObjectInstance> object):
	guarded(false),
	value(0)
{
	addInstance(object);
}

Object::Object(const Object & object):
	guarded(false),
	value(object.value)
{
	for(const auto & i : object.dInstances)
		addInstance(i.pointer(), i.getPosition());
	setPosition(object.getPosition());
}

std::list<Object::Instance*> & Object::instances()
{
	if (cachedInstanceList.empty())
	{
		for(auto & i : dInstances)
			cachedInstanceList.push_back(&i);
	}
	return cachedInstanceList;
}

std::list<const Object::Instance*> & Object::instances() const
{
	if (cachedInstanceConstList.empty())
	{
		for(const auto & i : dInstances)
			cachedInstanceConstList.push_back(&i);
	}
	return cachedInstanceConstList;
}

void Object::addInstance(Instance & object)
{
	//assert(object.dParent == *this);
	setGuardedIfMonster(object);
	dInstances.push_back(object);
	cachedInstanceList.push_back(&object);
	cachedInstanceConstList.push_back(&object);

	clearCachedArea();
	visibleTopOffset.reset();
}

Object::Instance & Object::addInstance(std::shared_ptr<CGObjectInstance> object)
{
	dInstances.emplace_back(*this, object);
	setGuardedIfMonster(dInstances.back());
	cachedInstanceList.push_back(&dInstances.back());
	cachedInstanceConstList.push_back(&dInstances.back());

	clearCachedArea();
	visibleTopOffset.reset();
	return dInstances.back();
}

Object::Instance & Object::addInstance(std::shared_ptr<CGObjectInstance> object, const int3 & position)
{
	dInstances.emplace_back(*this, object, position);
	setGuardedIfMonster(dInstances.back());
	cachedInstanceList.push_back(&dInstances.back());
	cachedInstanceConstList.push_back(&dInstances.back());

	clearCachedArea();
	visibleTopOffset.reset();
	return dInstances.back();
}

bool Object::isVisitable() const
{
	return vstd::contains_if(dInstances, [](const Instance & instance)
	{
		return instance.object().isVisitable();
	});
}

const int3 & Object::getPosition() const
{
	return dPosition;
}

int3 Object::getVisitablePosition() const
{
	assert(!dInstances.empty());

	// FIXME: This doesn't take into account Hota monster offset
	if (guarded)
		return getGuardPos();

	for(const auto & instance : dInstances)
		if(!getArea().contains(instance.getVisitablePosition()))
			return instance.getVisitablePosition();
	
	return dInstances.back().getVisitablePosition(); //fallback - return position of last object
}

const rmg::Area & Object::getAccessibleArea(bool exceptLast) const
{
	if(dInstances.empty())
		return dAccessibleAreaFullCache;
	if(exceptLast && !dAccessibleAreaCache.empty())
		return dAccessibleAreaCache;
	if(!exceptLast && !dAccessibleAreaFullCache.empty())
		return dAccessibleAreaFullCache;

	// FIXME: This clears tiles for every consecutive object
	for(auto i = dInstances.begin(); i != std::prev(dInstances.end()); ++i)
		dAccessibleAreaCache.unite(i->getAccessibleArea());

	dAccessibleAreaFullCache = dAccessibleAreaCache;
	dAccessibleAreaFullCache.unite(dInstances.back().getAccessibleArea());
	dAccessibleAreaCache.subtract(getArea());
	dAccessibleAreaFullCache.subtract(getArea());

	if(exceptLast)
		return dAccessibleAreaCache;
	else
		return dAccessibleAreaFullCache;
}

const rmg::Area & Object::getBlockVisitableArea() const
{
	if(dBlockVisitableCache.empty())
	{
		for(const auto & i : dInstances)
		{
			// FIXME: Account for blockvis objects with multiple visitable tiles
			if (i.isBlockedVisitable())
				dBlockVisitableCache.add(i.getVisitablePosition());
		}
	}
	return dBlockVisitableCache;
}

const rmg::Area & Object::getRemovableArea() const
{
	if(dRemovableAreaCache.empty())
	{
		for(const auto & i : dInstances)
		{
			if (i.isRemovable())
				dRemovableAreaCache.unite(i.getBlockedArea());
		}
	}

	return dRemovableAreaCache;
}

const rmg::Area & Object::getVisitableArea() const
{
	if(dVisitableCache.empty())
	{
		for(const auto & i : dInstances)
		{
			// FIXME: Account for objects with multiple visitable tiles
			dVisitableCache.add(i.getVisitablePosition());
		}
	}
	return dVisitableCache;
}

const rmg::Area Object::getEntrableArea() const
{
	// Calculate Area that hero can freely pass

	// Do not use blockVisitTiles, unless they belong to removable objects (resources etc.)
	// area = accessibleArea - (blockVisitableArea - removableArea)

	// FIXME: What does it do? AccessibleArea means area AROUND the object 
	rmg::Area entrableArea = getVisitableArea();
	rmg::Area blockVisitableArea = getBlockVisitableArea();
	blockVisitableArea.subtract(getRemovableArea());
	entrableArea.subtract(blockVisitableArea);

	return entrableArea;
}

void Object::setPosition(const int3 & position)
{
	auto shift = position - dPosition;

	dAccessibleAreaCache.translate(shift);
	dAccessibleAreaFullCache.translate(shift);
	dBlockVisitableCache.translate(shift);
	dVisitableCache.translate(shift);
	dRemovableAreaCache.translate(shift);
	dFullAreaCache.translate(shift);
	dBorderAboveCache.translate(shift);
	
	dPosition = position;
	for(auto& i : dInstances)
		i.setPositionRaw(i.getPosition());
}

void Object::setTemplate(const TerrainId & terrain, vstd::RNG & rng)
{
	for(auto& i : dInstances)
		i.setTemplate(terrain, rng);

	visibleTopOffset.reset();
}

const Area & Object::getArea() const
{
	if(!dFullAreaCache.empty() || dInstances.empty())
		return dFullAreaCache;
	
	for(const auto & instance : dInstances)
	{
		dFullAreaCache.unite(instance.getBlockedArea());
	}
	
	return dFullAreaCache;
}

const Area & Object::getBorderAbove() const
{
	if(dBorderAboveCache.empty())
	{
		for(const auto & instance : dInstances)
		{
			if (instance.isRemovable() || instance.object().appearance->isVisitableFromTop())
				continue;
			dBorderAboveCache.unite(instance.getBorderAbove());
		}
	}
	return dBorderAboveCache;
}

const int3 Object::getVisibleTop() const
{
	if (visibleTopOffset)
	{
		return dPosition + visibleTopOffset.value();
	}
	else
	{
		int3 topTile(-1, 10000, -1); //Start at the bottom
		for (const auto& i : dInstances)
		{
			if (i.getTopTile().y < topTile.y)
			{
				topTile = i.getTopTile();
			}
		}
		visibleTopOffset = topTile - dPosition;
		return topTile;
	}
}

bool rmg::Object::isGuarded() const
{
	return guarded;
}

void rmg::Object::setGuardedIfMonster(const Instance& object)
{
	if (object.object().ID == Obj::MONSTER)
	{
		guarded = true;
	}
}

int3 rmg::Object::getGuardPos() const
{
	if (guarded)
	{
		for (auto & instance : dInstances)
		{
			if (instance.object().ID == Obj::MONSTER)
			{
				return instance.getVisitablePosition();
			}
		}
	}
	return int3(-1,-1,-1);
}

void rmg::Object::setValue(uint32_t newValue)
{
	value = newValue;
}

uint32_t rmg::Object::getValue() const
{
	return value;
}

rmg::Area Object::Instance::getBorderAbove() const
{
	if (!object().isVisitable())
	{
		// TODO: Non-visitable objects don't need this, but theoretically could return valid area
		return rmg::Area();
	}

	int3 visitablePos = getVisitablePosition();
	auto areaVisitable = rmg::Area({visitablePos});
	auto borderAbove = areaVisitable.getBorderOutside();
	vstd::erase_if(borderAbove, [&](const int3 & tile)
	{
		return tile.y >= visitablePos.y ||
		(!object().blockingAt(tile + int3(0, 1, 0)) && 
		object().blockingAt(tile));
	});
	return borderAbove;
}

void Object::Instance::finalize(RmgMap & map, vstd::RNG & rng)
{
	if(!map.isOnMap(getPosition(true)))
		throw rmgException(boost::str(boost::format("Position of object %d at %s is outside the map") % dObject->id % getPosition(true).toString()));
	
	//If no specific template was defined for this object, select any matching
	if (!dObject->appearance)
	{
		const auto * terrainType = map.getTile(getPosition(true)).getTerrain();
		auto templates = dObject->getObjectHandler()->getTemplates(terrainType->getId());
		if (templates.empty())
		{
			throw rmgException(boost::str(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") % dObject->ID % dObject->getObjTypeIndex() % getPosition(true).toString() % terrainType));
		}
		else
		{
			setTemplate(terrainType->getId(), rng);
		}
	}

	if (dObject->isVisitable() && !map.isOnMap(dObject->visitablePos()))
		throw rmgException(boost::str(boost::format("Visitable tile %s of object %d at %s is outside the map") % dObject->visitablePos().toString() % dObject->id % dObject->anchorPos().toString()));

	for(const auto & tile : dObject->getBlockedPos())
	{
		if(!map.isOnMap(tile))
			throw rmgException(boost::str(boost::format("Tile %s of object %d at %s is outside the map") % tile.toString() % dObject->id % dObject->anchorPos().toString()));
	}

	for(const auto & tile : getBlockedArea().getTilesVector())
	{
		map.setOccupied(tile, ETileType::USED);
	}
	
	map.getMapProxy()->insertObject(dObject);
}

void Object::finalize(RmgMap & map, vstd::RNG & rng)
{
	if(dInstances.empty())
		throw rmgException("Cannot finalize object without instances");

	for(auto & dInstance : dInstances)
	{
		dInstance.finalize(map, rng);
	}
}

void Object::clearCachedArea() const
{
	dFullAreaCache.clear();
	dAccessibleAreaCache.clear();
	dAccessibleAreaFullCache.clear();
	dBlockVisitableCache.clear();
	dVisitableCache.clear();
	dRemovableAreaCache.clear();
	dBorderAboveCache.clear();
}

void Object::clear()
{
	for(auto & instance : dInstances)
		instance.clear();
	dInstances.clear();
	cachedInstanceList.clear();
	cachedInstanceConstList.clear();
	visibleTopOffset.reset();

	clearCachedArea();
}
 

VCMI_LIB_NAMESPACE_END
