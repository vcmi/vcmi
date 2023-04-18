/*
* MapObjectsEvaluator.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once
#include "../../lib/mapObjects/CObjectClassesHandler.h"

class MapObjectsEvaluator
{
private:
	std::map<CompoundMapObjectID, int> objectDatabase; //value for each object type

public:
	MapObjectsEvaluator();
	static MapObjectsEvaluator & getInstance();
	std::optional<int> getObjectValue(int primaryID, int secondaryID) const;
	std::optional<int> getObjectValue(const CGObjectInstance * obj) const;
	void addObjectData(int primaryID, int secondaryID, int value);
	void removeObjectData(int primaryID, int secondaryID);
};

