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

struct AiMapObjectID
{
	int primaryID;
	int secondaryID;

	AiMapObjectID(int primID, int secID) : primaryID(primID), secondaryID(secID) {};
};

inline bool operator<(const AiMapObjectID& obj1, const AiMapObjectID& obj2)
{
	if(obj1.primaryID != obj2.primaryID)
		return obj1.primaryID < obj2.primaryID;
	else
		return obj1.secondaryID < obj2.secondaryID;
}

inline bool operator==(const AiMapObjectID& obj1, const AiMapObjectID& obj2)
{
	if(obj1.primaryID == obj2.primaryID)
		return obj1.secondaryID == obj2.secondaryID;

	return false;
}

class MapObjectsEvaluator
{
private:
	std::map<AiMapObjectID, int> objectDatabase; //value for each object type
	static std::unique_ptr<MapObjectsEvaluator> singletonInstance;

public:
	MapObjectsEvaluator();
	static MapObjectsEvaluator & getInstance();
	boost::optional<int> getObjectValue(int primaryID, int secondaryID);
	void addObjectData(int primaryID, int secondaryID, int value);
	void removeObjectData(int primaryID, int secondaryID, int value);
};

