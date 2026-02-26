/*
 * RmgObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../int3.h"
#include "RmgArea.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

class CGObjectInstance;
class RmgMap;

namespace rmg {
class Object
{
public:
	
	class Instance
	{
	public:
		Instance(const Object& parent, std::shared_ptr<CGObjectInstance> object);
		Instance(const Object& parent, std::shared_ptr<CGObjectInstance> object, const int3 & position);
		
		const Area & getBlockedArea() const;
		
		int3 getVisitablePosition() const;
		bool isVisitableFrom(const int3 & tile) const;
		bool isBlockedVisitable() const;
		bool isRemovable() const;
		const Area & getAccessibleArea() const;
		Area getBorderAbove() const;
		void setTemplate(TerrainId terrain, vstd::RNG &); //cache invalidation
		void setAnyTemplate(vstd::RNG &); //cache invalidation
		
		int3 getTopTile() const;
		int3 getPosition(bool isAbsolute = false) const;
		void setPosition(const int3 & position); //cache invalidation
		void setPositionRaw(const int3 & position); //no cache invalidation
		const CGObjectInstance & object() const;
		CGObjectInstance & object();
		std::shared_ptr<CGObjectInstance> pointer() const;
		
		void finalize(RmgMap & map, vstd::RNG &); //cache invalidation
		void clear();
		
		std::function<void(CGObjectInstance &)> onCleared;
	private:
		mutable Area dBlockedAreaCache;
		int3 dPosition;
		mutable Area dAccessibleAreaCache;
		std::shared_ptr<CGObjectInstance> dObject;
		const Object & dParent;
	};
	
	Object() = default;
	Object(const Object & object);
	Object(std::shared_ptr<CGObjectInstance> object);
	Object(std::shared_ptr<CGObjectInstance> object, const int3 & position);
	
	void addInstance(Instance & object);
	Instance & addInstance(std::shared_ptr<CGObjectInstance> object);
	Instance & addInstance(std::shared_ptr<CGObjectInstance> object, const int3 & position);
	
	std::list<Instance*> & instances();
	std::list<const Instance*> & instances() const;
	
	int3 getVisitablePosition() const;
	const Area & getAccessibleArea(bool exceptLast = false) const;
	const Area & getBlockVisitableArea() const;
	const Area & getVisitableArea() const;
	const Area & getRemovableArea() const;
	const Area getEntrableArea() const;
	const Area & getBorderAbove() const;
	
	bool isVisitable() const;
	const int3 & getPosition() const;
	void setPosition(const int3 & position);
	void setTemplate(const TerrainId & terrain, vstd::RNG &);
	
	const Area & getArea() const;  //lazy cache invalidation
	const int3 getVisibleTop() const;

	bool isGuarded() const;
	int3 getGuardPos() const;
	void setGuardedIfMonster(const Instance & object);
	void setValue(uint32_t value);
	uint32_t getValue() const;
	
	void finalize(RmgMap & map, vstd::RNG &);
	void clearCachedArea() const;
	void clear();
	
private:
	std::list<Instance> dInstances;
	mutable Area dFullAreaCache;
	mutable Area dAccessibleAreaCache;
	mutable Area dAccessibleAreaFullCache;
	mutable Area dBlockVisitableCache;
	mutable Area dVisitableCache;
	mutable Area dRemovableAreaCache;
	mutable Area dBorderAboveCache;
	int3 dPosition;
	mutable std::optional<int3> visibleTopOffset;
	mutable std::list<Object::Instance*> cachedInstanceList;
	mutable std::list<const Object::Instance*> cachedInstanceConstList;
	bool guarded;
	uint32_t value;
};
}

VCMI_LIB_NAMESPACE_END
