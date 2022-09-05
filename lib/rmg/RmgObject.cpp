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
#include "../mapObjects/CObjectHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "Functions.h"

using namespace rmg;

Object::Instance::Instance(const Object& parent, CGObjectInstance & object): dParent(parent), dObject(object)
{
	setPosition(dPosition);
}

Object::Instance::Instance(const Object& parent, CGObjectInstance & object, const int3 & position): Instance(parent, object)
{
	setPosition(position);
}

const Area & Object::Instance::getBlockedArea() const
{
	if(dBlockedAreaCache.empty())
	{
		dBlockedAreaCache.assign(dObject.getBlockedPos());
		if(dObject.isVisitable() || dBlockedAreaCache.empty())
			dBlockedAreaCache.add(dObject.visitablePos());
	}
	return dBlockedAreaCache;
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
	return dObject.visitablePos();
}

const rmg::Area & Object::Instance::getAccessibleArea() const
{
	if(dAccessibleAreaCache.empty())
	{
		auto neighbours = rmg::Area({getVisitablePosition()}).getBorderOutside();
		rmg::Area visitable = rmg::Area(neighbours) - getBlockedArea();
		for(auto & from : visitable.getTiles())
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
	dObject.pos = dPosition + dParent.getPosition();
	
	dBlockedAreaCache.clear();
	dAccessibleAreaCache.clear();
	dParent.dAccessibleAreaCache.clear();
	dParent.dAccessibleAreaFullCache.clear();
	dParent.dFullAreaCache.clear();
}

void Object::Instance::setPositionRaw(const int3 & position)
{
	if(!dObject.pos.valid())
	{
		dObject.pos = dPosition + dParent.getPosition();
		dBlockedAreaCache.clear();
		dAccessibleAreaCache.clear();
		dParent.dAccessibleAreaCache.clear();
		dParent.dAccessibleAreaFullCache.clear();
		dParent.dFullAreaCache.clear();
	}
		
	auto shift = position + dParent.getPosition() - dObject.pos;
	
	dAccessibleAreaCache.translate(shift);
	dBlockedAreaCache.translate(shift);
	
	dPosition = position;
	dObject.pos = dPosition + dParent.getPosition();
}

void Object::Instance::setTemplate(const Terrain & terrain)
{
	if(dObject.appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(dObject.ID, dObject.subID)->getTemplates(terrain);
		if(templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") % dObject.ID % dObject.subID % static_cast<std::string>(terrain)));
		
		dObject.appearance = templates.front();
	}
	dAccessibleAreaCache.clear();
	setPosition(getPosition(false));
}

void Object::Instance::clear()
{
	delete &dObject;
	dBlockedAreaCache.clear();
	dAccessibleAreaCache.clear();
	dParent.dAccessibleAreaCache.clear();
	dParent.dAccessibleAreaFullCache.clear();
	dParent.dFullAreaCache.clear();
}

bool Object::Instance::isVisitableFrom(const int3 & position) const
{
	auto relPosition = position - getPosition(true);
	return dObject.appearance.isVisitableFrom(relPosition.x, relPosition.y);
}

CGObjectInstance & Object::Instance::object()
{
	return dObject;
}

const CGObjectInstance & Object::Instance::object() const
{
	return dObject;
}

Object::Object(CGObjectInstance & object, const int3 & position)
{
	addInstance(object, position);
}

Object::Object(CGObjectInstance & object)
{
	addInstance(object);
}

Object::Object(const Object & object)
{
	dStrenght = object.dStrenght;
	for(auto & i : object.dInstances)
		addInstance(const_cast<CGObjectInstance &>(i.object()), i.getPosition());
	setPosition(object.getPosition());
}

std::list<Object::Instance*> Object::instances()
{
	std::list<Object::Instance*> result;
	for(auto & i : dInstances)
		result.push_back(&i);
	return result;
}

std::list<const Object::Instance*> Object::instances() const
{
	std::list<const Object::Instance*> result;
	for(const auto & i : dInstances)
		result.push_back(&i);
	return result;
}

void Object::addInstance(Instance & object)
{
	//assert(object.dParent == *this);
	dInstances.push_back(object);
	dFullAreaCache.clear();
	dAccessibleAreaCache.clear();
	dAccessibleAreaFullCache.clear();
}

Object::Instance & Object::addInstance(CGObjectInstance & object)
{
	dInstances.emplace_back(*this, object);
	dFullAreaCache.clear();
	dAccessibleAreaCache.clear();
	dAccessibleAreaFullCache.clear();
	return dInstances.back();
}

Object::Instance & Object::addInstance(CGObjectInstance & object, const int3 & position)
{
	dInstances.emplace_back(*this, object, position);
	dFullAreaCache.clear();
	dAccessibleAreaCache.clear();
	dAccessibleAreaFullCache.clear();
	return dInstances.back();
}

const int3 & Object::getPosition() const
{
	return dPosition;
}

int3 Object::getVisitablePosition() const
{
	assert(!dInstances.empty());
	for(auto & instance : dInstances)
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

void Object::setPosition(const int3 & position)
{
	dAccessibleAreaCache.translate(position - dPosition);
	dAccessibleAreaFullCache.translate(position - dPosition);
	dFullAreaCache.translate(position - dPosition);
	
	dPosition = position;
	for(auto& i : dInstances)
		i.setPositionRaw(i.getPosition());
}

void Object::setTemplate(const Terrain & terrain)
{
	for(auto& i : dInstances)
		i.setTemplate(terrain);
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

void Object::Instance::finalize(RmgMap & map)
{
	if(!map.isOnMap(getPosition(true)))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % dObject.id % getPosition(true).toString()));
	
	if (dObject.isVisitable() && !map.isOnMap(dObject.visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % dObject.visitablePos().toString() % dObject.id % dObject.pos.toString()));
	
	for (auto & tile : dObject.getBlockedPos())
	{
		if(!map.isOnMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile.toString() % dObject.id % dObject.pos.toString()));
	}
	
	if (dObject.appearance.id == Obj::NO_OBJ)
	{
		auto terrainType = map.map().getTile(getPosition(true)).terType;
		auto templates = VLC->objtypeh->getHandlerFor(dObject.ID, dObject.subID)->getTemplates(terrainType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") % dObject.ID % dObject.subID % getPosition(true).toString() % terrainType));
		
		setTemplate(terrainType);
	}
	
	for(auto & tile : getBlockedArea().getTilesVector())
	{
		map.setOccupied(tile, ETileType::ETileType::USED);
	}
	
	map.getEditManager()->insertObject(&dObject);
}

void Object::finalize(RmgMap & map)
{
	if(dInstances.empty())
		throw rmgException("Cannot finalize object without instances");
	
	for(auto iter = dInstances.begin(); iter != dInstances.end(); ++iter)
	{
		iter->finalize(map);
	}
}

void Object::clear()
{
	for(auto & instance : dInstances)
		instance.clear();
	dInstances.clear();
	dFullAreaCache.clear();
	dAccessibleAreaCache.clear();
	dAccessibleAreaFullCache.clear();
}
 
