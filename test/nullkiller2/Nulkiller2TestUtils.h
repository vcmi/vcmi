/*
* Nulkiller2TestUtils.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*/

#include "ResourceSet.h"

class Nulkiller2TestUtils
{
public:
	static TResources
	res(const int wood, const int mercury, const int ore, const int sulfur, const int crystals, const int gems, const int gold)
	{
		TResources resources;
		resources[0] = wood;
		resources[1] = mercury;
		resources[2] = ore;
		resources[3] = sulfur;
		resources[4] = crystals;
		resources[5] = gems;
		resources[6] = gold;
		return resources;
	}
};