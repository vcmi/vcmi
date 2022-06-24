/*
 * CRmgObject.h, part of VCMI engine
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
#include "CRmgArea.h"
#include "CMapGenerator.h"

class CGObjectInstance;
class CMapGenerator;

namespace Rmg {
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
		bool isVisitableFrom(const int3 & tile, const int3 & potentialPosition) const;
		void setTemplate(ETerrainType terrain);
		
		const int3 & getPosition() const;
		void setPosition(const int3 & position);
		const CGObjectInstance & object() const;
		CGObjectInstance & object();
		
		void finalize(CMapGenerator & generator);
		
	private:
		Area dBlockedArea;
		int3 dPosition;
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
	
	const int3 & getPosition() const;
	void setPosition(const int3 & position);
	void setTemplate(ETerrainType terrain);
	
	const Area & getArea() const;  //lazy cache invalidation
	
	void finalize(CMapGenerator & generator);
	
private:
	std::list<Instance> dInstances;
	mutable Area dFullAreaCache;
	int3 dPosition;
	ui32 dStrenght;
};
}
