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

class CGObjectInstance;
class RmgMap;

namespace rmg {
class Object
{
public:
	
	class Instance
	{
	public:
		Instance(const Object& parent, CGObjectInstance & object);
		Instance(const Object& parent, CGObjectInstance & object, const int3 & position);
		
		const Area & getBlockedArea() const;
		
		int3 getVisitablePosition() const;
		bool isVisitableFrom(const int3 & tile) const;
		const Area & getAccessibleArea() const;
		void setTemplate(const TTerrainId & terrain); //cache invalidation
		
		int3 getPosition(bool isAbsolute = false) const;
		void setPosition(const int3 & position); //cache invalidation
		void setPositionRaw(const int3 & position); //no cache invalidation
		const CGObjectInstance & object() const;
		CGObjectInstance & object();
		
		void finalize(RmgMap & map); //cache invalidation
		void clear();
		
	private:
		mutable Area dBlockedAreaCache;
		int3 dPosition;
		mutable Area dAccessibleAreaCache;
		CGObjectInstance & dObject;
		const Object & dParent;
	};
	
	Object() = default;
	Object(const Object & object);
	Object(CGObjectInstance & object);
	Object(CGObjectInstance & object, const int3 & position);
	
	void addInstance(Instance & object);
	Instance & addInstance(CGObjectInstance & object);
	Instance & addInstance(CGObjectInstance & object, const int3 & position);
	
	std::list<Instance*> instances();
	std::list<const Instance*> instances() const;
	
	int3 getVisitablePosition() const;
	const Area & getAccessibleArea(bool exceptLast = false) const;
	
	const int3 & getPosition() const;
	void setPosition(const int3 & position);
	void setTemplate(const TTerrainId & terrain);
	
	const Area & getArea() const;  //lazy cache invalidation
	
	void finalize(RmgMap & map);
	void clear();
	
private:
	std::list<Instance> dInstances;
	mutable Area dFullAreaCache;
	mutable Area dAccessibleAreaCache, dAccessibleAreaFullCache;
	int3 dPosition;
	ui32 dStrenght;
};
}

VCMI_LIB_NAMESPACE_END
