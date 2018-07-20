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

class MapObjectsEvaluator
{
private:
	static std::map<int, std::map<int, int>> objectDatabase; //each object contains map of subobjects with their values

public:
	static void init();
	static boost::optional<int> getObjectValue(int primaryID, int secondaryID);
};

